# Réponses de l'examen de Tanguy Rolland


## lscpu

```
Architecture:                    x86_64
CPU op-mode(s):                  32-bit, 64-bit
Byte Order:                      Little Endian
Address sizes:                   39 bits physical, 48 bits virtual
CPU(s):                          4
On-line CPU(s) list:             0-3
Thread(s) per core:              1
Core(s) per socket:              4
Socket(s):                       1
NUMA node(s):                    1
Vendor ID:                       GenuineIntel
CPU family:                      6
Model:                           158
Model name:                      Intel(R) Core(TM) i5-9300H CPU @ 2.40GHz
Stepping:                        10
CPU MHz:                         2400.032
BogoMIPS:                        4800.06
Hypervisor vendor:               Oracle
Virtualization type:             full
L1d cache:                       128 KiB
L1i cache:                       128 KiB
L2 cache:                        1 MiB
L3 cache:                        32 MiB
NUMA node0 CPU(s):               0-3
```

# Question 2 : Mesure du temps

Avec un pourcentage de compression de 10%, pour l'image small_lena_gray.png, j'obtiens les résultats suivants : 

Temps encodage DFT : 114.653s
Temps selection pixels : 0.00563309s
Temps reconstitution image : 15.1799s

et pour l'image tiny_lena_gray.png, j'obtiens : 

Temps encodage DFT : 0.431607s
Temps selection pixels : 0.000314839s
Temps reconstitution image : 0.0626178s


# Parallélisation avec OpenMP

Pour la boucle d'encodage DFT parallélisée avec #pragma omp parallel for num_threads(NB_THREADS)
Sur l'image small_lena_gray.png

NB_threads | temps moyen simulation| speedup global
-----------|-----------------------|---------------
2          |     83.0696s          |     1.4
3          |     56.0319s          |      2
4          |     41.1409s          |     2.8

De même sur la boucle de reconstitution, on obtient un speedup maximal de 3.9 pour 4 threads OpenMP

Dans les deux cas, la parallélisation se fait sur les pixels puisqu'on répartit un certain nombre d'indices par boucle (partition statique) sur chaque thread. Après l'avoir essayée, la partition dynamique OMP n'accélère pas plus ces deux étapes.

Il est inutile de paralléléliser pour l'instant la boucle de séléction des pixels car son temps d'éxecution est négligeable devant celui des deux autres boucles.

# MPI 1

Je n'ai pas eu le temps de faire la mesure d'accélération.

















