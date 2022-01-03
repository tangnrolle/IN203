#include <cstdlib>
#include <random>
#include <iostream>
#include <fstream>
#include <chrono>
#include <mpi.h>
#include "contexte.hpp"
#include "individu.hpp"
#include "graphisme/src/SDL2/sdl2.hpp"

void majStatistique(epidemie::Grille &grille, std::vector<epidemie::Individu> const &individus)
{
    for (auto &statistique : grille.getStatistiques())
    {
        statistique.nombre_contaminant_grippe_et_contamine_par_agent = 0;
        statistique.nombre_contaminant_seulement_contamine_par_agent = 0;
        statistique.nombre_contaminant_seulement_grippe = 0;
    }
    auto [largeur, hauteur] = grille.dimension();
    auto &statistiques = grille.getStatistiques();
    for (auto const &personne : individus)
    {
        auto pos = personne.position();

        std::size_t index = pos.x + pos.y * largeur;
        if (personne.aGrippeContagieuse())
        {
            if (personne.aAgentPathogeneContagieux())
            {
                statistiques[index].nombre_contaminant_grippe_et_contamine_par_agent += 1;
            }
            else
            {
                statistiques[index].nombre_contaminant_seulement_grippe += 1;
            }
        }
        else
        {
            if (personne.aAgentPathogeneContagieux())
            {
                statistiques[index].nombre_contaminant_seulement_contamine_par_agent += 1;
            }
        }
    }
}

void afficheSimulation(sdl2::window &ecran, epidemie::Grille const &grille, std::size_t jour)
{
    auto [largeur_ecran, hauteur_ecran] = ecran.dimensions();
    auto [largeur_grille, hauteur_grille] = grille.dimension();
    auto const &statistiques = grille.getStatistiques();
    sdl2::font fonte_texte("./graphisme/src/data/Lato-Thin.ttf", 18);
    ecran.cls({0x00, 0x00, 0x00});
    // Affichage de la grille :
    std::uint16_t stepX = largeur_ecran / largeur_grille;
    unsigned short stepY = (hauteur_ecran - 50) / hauteur_grille;
    double factor = 255. / 15.;

    for (unsigned short i = 0; i < largeur_grille; ++i)
    {
        for (unsigned short j = 0; j < hauteur_grille; ++j)
        {
            auto const &stat = statistiques[i + j * largeur_grille];
            int valueGrippe = stat.nombre_contaminant_grippe_et_contamine_par_agent + stat.nombre_contaminant_seulement_grippe;
            int valueAgent = stat.nombre_contaminant_grippe_et_contamine_par_agent + stat.nombre_contaminant_seulement_contamine_par_agent;
            std::uint16_t origx = i * stepX;
            std::uint16_t origy = j * stepY;
            std::uint8_t red = valueGrippe > 0 ? 127 + std::uint8_t(std::min(128., 0.5 * factor * valueGrippe)) : 0;
            std::uint8_t green = std::uint8_t(std::min(255., factor * valueAgent));
            std::uint8_t blue = std::uint8_t(std::min(255., factor * valueAgent));
            ecran << sdl2::rectangle({origx, origy}, {stepX, stepY}, {red, green, blue}, true);
        }
    }

    ecran << sdl2::texte("Carte population grippee", fonte_texte, ecran, {0xFF, 0xFF, 0xFF, 0xFF}).at(largeur_ecran / 2, hauteur_ecran - 20);
    ecran << sdl2::texte(std::string("Jour : ") + std::to_string(jour), fonte_texte, ecran, {0xFF, 0xFF, 0xFF, 0xFF}).at(0, hauteur_ecran - 20);
    ecran << sdl2::flush;
}

void simulation(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    MPI_Comm globComm;
    MPI_Comm_dup(MPI_COMM_WORLD, &globComm);
    int nbp;
    MPI_Comm_size(globComm, &nbp);
    int rank;
    MPI_Comm_rank(globComm, &rank);

    // On doit créer un nouveau canal de communication pour tous les autres process que l'affichage pour que le Gather se fasse bien.
    MPI_Comm splitComm;
    int color = rank != 0 ? 0 : 1;
    MPI_Comm_split(globComm, color, rank, &splitComm);

    MPI_Status status;
    MPI_Status quitStatus;
    MPI_Request quitRequest, sendRequest;

    constexpr const unsigned int largeur_ecran = 1280, hauteur_ecran = 1024;
    sdl2::window ecran("Simulation epidemie de grippe", {largeur_ecran, hauteur_ecran});

    // On ferme la fenêtre ouverte par les processus != 0 pour ne garder que celle de l'affichage
    if (rank != 0)
    {
        sdl2::finalize();
    }

    epidemie::ContexteGlobal contexte;
    // contexte.deplacement_maximal = 1; <= Si on veut moins de brassage
    // contexte.taux_population = 400'000;
    // contexte.taux_population = 1'000;
    contexte.interactions.beta = 60.;
    epidemie::Grille grille{contexte.taux_population};

    auto [largeur_grille, hauteur_grille] = grille.dimension();

    std::size_t jours_ecoules = 0;

    bool quitting = false;

    std::chrono::time_point<std::chrono::system_clock> start_day, end_day, end_day_without_display;
    std::chrono::duration<double> mean_simulation_time;

    int quit_flag, send_flag;
    bool exit_display = false;

    // On met les processus de rang > 0 en état d'alerte permanent pour se fermer quand on quitte l'affichage (processus n°0)

    // Grille temporaire qui servira aux deux processus
    std::vector<epidemie::Grille::StatistiqueParCase> grille_tmp;
    grille_tmp.reserve(contexte.taux_population);
    grille_tmp = grille.getStatistiques();

    if (rank > 0)
    {
        // On met les processus de rang > 0 en état d'alerte permanent pour se fermer quand on quitte l'affichage (processus n°0)
        MPI_Irecv(&exit_display, 1, MPI_C_BOOL, 0, 1, globComm, &quitRequest);

        unsigned int graine_aleatoire = 1;
        std::uniform_real_distribution<double> porteur_pathogene(0., 1.);

        // On répartit la population et le nombre d'immunisés équitablement entre les processus, sauf pour le dernier
        // auquel on affecte également le reste.
        // De même pour le nombre de nouvelles contaminations chaque jour.

        int tx_immunises = (contexte.taux_population * 23) / 100;
        size_t partition_population;
        int nombre_immunises_grippe;
        int new_contamine;

        if (rank != nbp - 1)
        {
            partition_population = contexte.taux_population / nbp;
            nombre_immunises_grippe = (partition_population * 23) / 100;
            new_contamine = 25 / nbp;
        }

        else
        {
            partition_population = (contexte.taux_population / nbp) + (contexte.taux_population % nbp);
            nombre_immunises_grippe = tx_immunises - (nbp - 1) * (partition_population * 23) / 100;
            new_contamine = 25 - (nbp - 1) * 25 / nbp;
        }

        float temps = 0;
        std::vector<epidemie::Individu> population;
        population.reserve(partition_population);

        // L'agent pathogene n'evolue pas et reste donc constant...
        epidemie::AgentPathogene agent(graine_aleatoire++);
        // Initialisation de la population initiale pour chaque processus :
        graine_aleatoire += (rank - 1) * partition_population;
        for (std::size_t i = 0; i < partition_population; ++i)
        {
            std::default_random_engine motor(100 * (i + 1));
            population.emplace_back(graine_aleatoire++, contexte.esperance_de_vie, contexte.deplacement_maximal);
            population.back().setPosition(largeur_grille, hauteur_grille);
            if (porteur_pathogene(motor) < 0.2)
            {
                population.back().estContamine(agent);
            }
        }

        int flag = 0;
        int jour_apparition_grippe = 0;

        epidemie::Grippe grippe(0);
        std::ofstream output("Courbe.dat");

        if (rank == 1)
        {
            std::cout << "Debut boucle epidemie" << std::endl
                      << std::flush;
        }

        while (!quitting)
        {
            start_day = std::chrono::system_clock::now();

            if (jours_ecoules % 365 == 0) // Si le premier Octobre (debut de l'annee pour l'epidemie ;-) )
            {
                grippe = epidemie::Grippe(jours_ecoules / 365);
                jour_apparition_grippe = grippe.dateCalculImportationGrippe();
                grippe.calculNouveauTauxTransmission();

                // 23% des gens sont immunises. On prend les 23% premiers
                for (int ipersonne = 0; ipersonne < nombre_immunises_grippe; ++ipersonne)
                {
                    population[ipersonne].devientImmuniseGrippe();
                }

                for (int ipersonne = nombre_immunises_grippe; ipersonne < int(partition_population); ++ipersonne)
                {
                    population[ipersonne].redevientSensibleGrippe();
                }
            }

            if (jours_ecoules % 365 == std::size_t(jour_apparition_grippe))
            {
                for (int ipersonne = nombre_immunises_grippe; ipersonne < nombre_immunises_grippe + new_contamine; ++ipersonne)
                {
                    population[ipersonne].estContamine(grippe);
                }
            }

            // Mise a jour des statistiques pour les cases de la grille :
            majStatistique(grille, population);
            auto &buffer = grille.getStatistiques();

            MPI_Allreduce(buffer.data(), grille_tmp.data(), largeur_grille * hauteur_grille * 3, MPI_INT, MPI_SUM, splitComm);
            grille.set_m_statistiques(grille_tmp);
            // On parcout la population pour voir qui est contamine et qui ne l'est pas, d'abord pour la grippe puis pour l'agent pathogene
            std::size_t compteur_grippe = 0, compteur_agent = 0, mouru = 0;
            for (auto &personne : population)
            {
                if (personne.testContaminationGrippe(grille, contexte.interactions, grippe, agent))
                {
                    compteur_grippe++;
                    personne.estContamine(grippe);
                }
                if (personne.testContaminationAgent(grille, agent))
                {
                    compteur_agent++;
                    personne.estContamine(agent);
                }
                // On verifie si il n'y a pas de personne qui veillissent de veillesse et on genere une nouvelle personne si c'est le cas.
                if (personne.doitMourir())
                {
                    mouru++;
                    unsigned nouvelle_graine = jours_ecoules + personne.position().x * personne.position().y;
                    personne = epidemie::Individu(nouvelle_graine, contexte.esperance_de_vie, contexte.deplacement_maximal);
                    personne.setPosition(largeur_grille, hauteur_grille);
                }
                personne.veillirDUnJour();
                personne.seDeplace(grille);
            }

            // Envoi asynchrone
            grille_tmp = grille.getStatistiques();
            MPI_Isend(grille_tmp.data(), largeur_grille * hauteur_grille * 3, MPI_INT, 0, 0, globComm, &sendRequest);

            end_day_without_display = std::chrono::system_clock::now();
            mean_simulation_time += end_day_without_display - start_day;
            output << jours_ecoules << "\t" << grille.nombreTotalContaminesGrippe() << "\t"
                   << grille.nombreTotalContaminesAgentPathogene() << std::endl;
            jours_ecoules += 1;

            MPI_Test(&quitRequest, &quit_flag, &quitStatus);
            if (quit_flag != 0)
            {
                quitting = true;
            }
        }
        output.close();

        if (rank == 1)
        {
            std::cout << "Temps calcul moyen simulation : " << mean_simulation_time.count() / jours_ecoules << "s\n";
        }
    }

    if (rank == 0)
    {
        while (!quitting)
        {
            start_day = std::chrono::system_clock::now();

            sdl2::event_queue queue;
            //#############################################################################################################
            //##    Affichage des resultats pour le temps  actuel
            //#############################################################################################################

            auto events = queue.pull_events();
            for (const auto &e : events)
            {
                if (e->kind_of_event() == sdl2::event::quit)
                {
                    quitting = true;
                    // On envoie également le message de fermeture de la fenêtre au processus de simulation n°1
                    exit_display = true;

                    for (int i = 1; i < nbp; i++)
                    {
                        MPI_Isend(&exit_display, 1, MPI_C_BOOL, i, 1, globComm, &quitRequest);
                    }
                    std::cout << "Broadcast quitting request\n";
                }
            }

            // Reception synchrone bloquée par Iprobe

            MPI_Iprobe(1, 0, globComm, &send_flag, &status);
            if (send_flag != 0)
            {
                MPI_Recv(grille_tmp.data(), largeur_grille * hauteur_grille * 3, MPI_INT, 1, 0, globComm, &status);
            }
            grille.set_m_statistiques(grille_tmp);
            afficheSimulation(ecran, grille, jours_ecoules);

            end_day = std::chrono::system_clock::now();
            std::chrono::duration<double> display_time = end_day - start_day;
            std::cout << "Temps calcul affichage : " << display_time.count() << "s\n";

            jours_ecoules += 1;
        }

    } // Fin boucle temporelle
}

int main(int argc, char *argv[])
{
    // parse command-line
    bool affiche = true;
    for (int i = 0; i < argc; i++)
    {
        std::cout << i << " " << argv[i] << "\n";
        if (std::string("-nw") == argv[i])
            affiche = false;
    }

    sdl2::init(argc, argv);
    {
        simulation(argc, argv);
    }
    sdl2::finalize();

    MPI_Finalize();
    return EXIT_SUCCESS;
}