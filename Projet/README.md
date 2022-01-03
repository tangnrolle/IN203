# Projet de Tanguy ROLLAND

# Question 2.1) simulation.cpp

On constate qu’en moyenne, le temps de simulation par pas de temps est de 0.10s, avec 80% consacré au calcul et 20% à l’affichage. Cela dépend bien sûr de l’efficacité de la carte graphique (Nvidia Geforce RTX 2060 pour ma part).

# Question 2.2) Affichage synchrone avec MPI

`mpirun -np 2 ./simulation_sync_affiche_mpi.exe`

Mon choix de parallélisation a été de faire une unique boucle while dans laquelle sont répartis les tâches entre les deux processus.
La condition d'arrêt se fait par l'envoi d'un booléen d'arrêt par le processus d'affichage (0) au processus de simulation (1), grâce à des envois/réceptions non-bloquants.

On observe les temps suivants: 

Temps calcul simulation parallele mpi: 0.0936434 s 
Temps affichage parallele mpi: 0.0912672 s

Temps de calcul simulation séquentiel : 0.0288338s
Temps affichage séquentiel : 0.0997003s
Temps total séquentiel : 0.12153s

Le programme parallèle synchrone avec MPI obtient un speedup plus petit que 1 pour le temps de simulation pure, qui est environ égal au temps d’affichage, celui-ci étant presque le même que le temps d’affichage en séquentiel. 

Ceci provient du fait que le synchronisme impose à la simulation d’”attendre” le processus d’affichage. Leurs temps sont donc les mêmes à la durée d’un envoi près.

Si on observe le temps d’éxécution total, on obtient un speedup de 1,3 puisque désormais le temps total correspond uniquement au temps d’affichage.

# Question 2.3) 

`mpirun -np 2 ./simulation_async_affiche_mpi.exe`

On obtient les résultats suivants : 

```
Temps calcul simulation : 0.0201096s
Temps calcul simulation : 0.0242698s
Temps calcul simulation : 0.0253432s
Temps calcul simulation : 0.0211458s
Temps calcul affichage : 0.101643s

```

Avec l’asynchronisme de l’affichage, on observe en moyenne 4 itérations de la simulation pour 1 affichage. Le MPI_Iprobe permet de ne pas générer de deadlock mais les envois non-bloquants impliquent aussi que que la simulation n’attend plus l’affichage. On a donc un affichage tous les 4 pas de temps donc moins précis mais nettement plus rapide.

On obtient un temps de simulation de 0.025s environ par pas de temps ce qui correspond au temps de calcul de la simulation en séquentiel. La simulation n’est donc plus limitée par l’affichage grâce à l’asynchronisme.

Le speedup global est donc d'environ S = 0.10 (Temps séquentiel) / (0.022(Moyenne du temps de simulation) + 1/4 * 0.11 (Moyenne du temps d'affichage par itération))
Soit S = 2.

Je n'ai cependant pas trouvé le moyen de placer la clause MPI_Iprobe sur l'envoi. Ainsi, la simulation envoie son résultat à chaque itération (envoi non-bloquant), mais l'affichage ne le reçoit que lorsqu'il est prêt à afficher de nouveau. Cependant ceci n'a que peu d'impact sur le temps de simulation puisqu'après mesure, le temps d'envoi est négligeable devant le temps de simulation.

# Question 2.4) 

`mpirun -np 2 ./simulation_async_omp.exe`

Après quelques tests, j'ai remarqué que seule la boucle for qui parcourt la population et teste l'état de contamination a un réel impact sur la durée d'éxécution lorsqu'elle est parallélisée. On la parallélise donc avec OpenMP. 

À nombre d'individus global constant (100 000), on obtient un temps de calcul moyen pour la simulation : 0.019777s
Le Speedup maximal sur la simulation est d'environ 1,3 obtenu pour 3 Threads OpenMP. C'est quasiment identique à la parallélisation sans OpenMP.

À nombre d'individu constant (100 000) par processus on obtient les résultats suivants : 

num_threads| temps moyen simulation| speedup global
-----------|-----------------------|---------------
     1     |     0.0194667s        |    5.2
     2     |     0.0406031s        |     5
     3     |     0.0635883s        |    4.73
     4     |     0.0862882s        |    4.75
     
On observe que le speedup global (en comptant l'affichage) est presque constant pour un nombre constant d'individus par processus. Il décroit cependant car il est limité par l'accès mémoire mais OpenMP s'avère très utile pour des grands nombres d'individus.

# Question 2.5) 

`mpirun -np x ./simulation_async_mpi.exe`

Pour cette question, il m'a fallu revoir l'organisation globale de mon programme. J'ai réorganisé mes boucles pour que désormais chaque processus déclenche sa propre boucle while. Cela permettait de distribuer plus facilement la population au différent processus. J'ai également dû reporter l'initialisation MPI directement dans la fonction de simulation pour pouvoir faire le MPI_Comm_split.

Pour répartir les individus, il a fallu diviser la population par le nombre de processus (en attribuant le potentiel reste au dernier processus), de même pour le nombre de personnes immunisées et de nouveaux contaminés par pas de temps. J'ai également attribué une graine par processus (générée en fonction de leur rang). On utilise ensuite un MPI_Allreduce avec une somme pour obtenir les statistiques de la population totale.

À nombre d'individus global constant (100 000), on obtient un speedup maximal de 7.5 sur le temps total, et un speedup maximal de 2 sur le temps de simulation obtenu pour 4 processus MPI. 

À nombre d'individu constant (100 000) par processus on obtient les résultats suivants : 

     np    | temps moyen simulation| speedup global
-----------|-----------------------|---------------
     2     |     0.0415859s        |     5
     3     |     0.039525s         |    7.5
     4     |     0.046782s         |    8.5

Cette fois-ci, le speedup augmente avec le nombre de processus à nombre d'individus constant par threads. C'est normal puisque les processus MPI fonctionne sur mémoire distribuée contrairement aux threads OMP qui sont sur mémoire partagée. Les processus MPI sont donc peu limités par l'accès mémoire et le speedup peut continuer d'augmenter quand on augmente le nombre de processus.

# Question 2.5.1)

`mpirun -np x ./simulation_async_mpi_omp.exe`

Avec cette dernière étape, à nombre d'individus global fixe, on obtient un speedup maximal qui vaut S = 7.5 pour 3 processus MPI et 1 unique thread OMP. Il semble qu'il faut trouver un équilibre de granularité entre ces deux parallélisation. En effet, si on met trop de threads OMP, on ralentit l'éxécution par des accès mémoire conflictuels et ceci sera d'autant plus marqué que le nombre de processus MPI est important. Le maximum obtenu est cohérent avec la présence de 4 coeurs sur ma machine, on en utilise 1 pour l'éxécution du programme et 3 pour la parallélisation. Il semble que l'utilisation de la mémoire partagée ralentit la simulation à nombre global d'individus constant sur ma machine.

À nombre d'individus constant par processus et par thread on obtient le tableau suivant:

     np    | temps moyen simulation| speedup global
-----------|-----------------------|---------------
     2     |     0.0409242s        |    5.1
     3     |     0.0314387s        |    6.8
     4     |     0.046782s         |     8

Le speedup augmente principalement grâce aux processus MPI mais l'augmentation du nombre de threads OMP semble à nouveau ralentir l'éxecution à cause de l'accès mémoire concurrentiel.

# Question 2.5.2) BILAN

La parallélisation la plus efficace sur ma machine est celle des processus MPI, c'est à dire sur la mémoire distribuée. L'allocation de la mémoire aux différents coeurs de ma machine virtuelle fait que les threads OpenMP ont plutôt tendance à ralentir l'éxecution par conflit d'accès à la mémoire partagée.

Ce projet m'a fait prendre conscience de l'importance des choix de parallélisation sur la vitesse d'éxecution et l'adaptabilité du programme. Mon choix d'ordre des boucles des premières questions s'est vite averé gênant pour la parallélisation de la simulation avec MPI et j'ai été forcé de l'inverser. Cela m'a également poussé à comprendre le comportement précis de ma machine lors de l'éxecution du programme. 







