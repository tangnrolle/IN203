

# TP3 de ROLLAND Tanguy

`pandoc -s --toc tp2.md --css=./github-pandoc.css -o tp2.html`





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

*Des infos utiles s'y trouvent : nb core, taille de cache*



## Produit scalaire 

*Expliquer les paramètres, les fichiers, l'optimisation de compil, NbSamples, ...*

On compile dotproduct.cpp avec le makefile et 1024 samples

OMP_NUM    | samples=1024 | accélération
-----------|--------------|----------
séquentiel |   0.10136    |    1
1          |   0.132091   |    0.76
2          |   0.0919012  |    1.1
3          |   0.0875006  |    1.14
4          |   0.0926789  |    1.1
8          |   0.183268   |    0.55


Mon ordinateur possède 4 coeurs et peut exécuter un unique thread par coeur. Or l'exécution du programme principal compte comme un thread. Ainsi, on optimise le temps de calcul avec OMP_NUM_THREADS=3, c'est-à-dire qu'on obtient la plus grande accélération. L'accélération reste très faible puisque la quantité de données traitées est petite et c'est donc principalement l'accès aux données qui prend du temps.

Dans le fichier dotproduct_thread.exe, on se débarasse des directives OPENMP et on travaille directement avec la bibliothèque "thread"

num_thread | temps de calcul | accélération
-----------|-----------------|----------
séquentiel |     0.315913    |    1
2          |     0.24734     |    1.3
3          |     0.266472    |    1.2
4          |     0.330906    |    0.9
8          |     0.598087    |    0.5

*Discuter sur ce qu'on observe, la logique qui s'y cache.*

Le temps de calcul plafonne à partir de 2 ou 3 processus en parallèle avec une légère accélération, puis diminue lorsqu'on continue à augmenter num_thread. Le programme perd en effet du temps dans la gestion des différents threads et le temps d'accès aux données n'est toujours pas négligeable. 

OpenMP semble donc être la solution optimale pour ce type de parallélisme : son optimisation en profondeur lui permet d'être respectivement plus rapide que les threads C++, et conserve également une accélération "positive" plus longtemps que les threads C++.


## Produit matrice-matrice

1)

dimension  | temps de calcul 
-----------|-----------------
1023       |    1.30998
1024       |	3.09533
1025       |    1.3012

### Permutation des boucles

*Expliquer comment est compilé le code (ligne de make ou de gcc) : on aura besoin de savoir l'optim, les paramètres, etc. Par exemple :*

`make TestProduct.exe && ./TestProductMatrix.exe 1024`


  ordre           | time    | MFlops  | MFlops(n=2048) 
------------------|---------|---------|----------------
i,j,k (origine)   | 2.73764 | 782.476 |
j,i,k             | 3.72883 | 575.913 |
i,k,j             | 12.0432 | 172.575 |
k,i,j             | 12.0822 | 177.910 |
j,k,i             | 0.45102 | 4761.64 | 3512.59
k,j,i             | 0.46310 | 4638.07 | 1930.87


*Discussion des résultats*

Les deux permutations qui placent l'indice i en dernier sont nettement plus efficaces en temps que les autres. En effet, cette permutation est celle qui exploite le mieux la mémoire cache. 
Comme expliqué dans les notes de cours, on effectue deux accès linéaires : un pour A et un pour C (on exploite les lignes de cache ou la mémoire entrelacée), tandis que B(k,j) est réutilisé. C'est l'option qui nécessite le moins de transfert de données. 

La taille de la matrice influence aussi le temps de calcul puisque les matrices plus petites tiennent intégralement dans la mémoire cache.



### OMP sur la meilleure boucle 

`make TestProductMatrix_omp.exe && OMP_NUM_THREADS=8 ./TestProductMatrix_omp.exe 1024`

On teste de parralleléliser la boucle j,k,i (meilleure boucle) grâce à OpenMP. On rajoute une ligne `#pragma omp parallel` avant la seconde boucle.

  OMP_NUM | temps (n=1024) | MFlops(n=1024) | MFlops(n=2048) | MFlops(n=512)  | MFlops(n=4096)
----------|----------------|----------------|----------------|----------------|---------------
1         |     0.8118     |     4840.25    |    2912.19     |    2777.49 2   |    9448.28 
2         |     0.4548     |     4720.9     |                |                |
3         |     0.3115     |     6892.27    |    13823.4     |                |
4         |     0.2674     |     8030.97    |    18040.6     |                | 
5         |     0.6357     |     3377.9     |    19009.2     |                |
6         |     0.6361     |     3375.77    |                |                |
7         |     0.6104     |     3517.92    |                |                |
8         |     0.6488     |     3309.86    |                |                |


Speedup : Le speedup vaut 1.78 pour 2 threads, 2.6 pour 3 threads, 3.0 pour 4 threads (optimal) puis descend sous 1.3 pour 5,6 et 7 threads. On retrouve l'effet de l'hyperthreading lorsque le nombre de threads et plus grand que le nombre de coeurs. Le speedup est presque une fonction linéaire du nombre de threads pour nb_threads <= 4.

Pour n=4096, on obtient un speedup maximal de 1.22

### Produit par blocs

`make TestProduct.exe && ./TestProduct.exe 1024`

  szBlock         | MFlops(n=1024)| MFlops(n=2048)|
------------------|---------------|---------------|
origine (=max)    |    2667.57    |    2441.42    |
32                |    2453.19    |               |
64                |    2020.9     |               |
128               |    2459.23    |               |
256               |    2272.48    |               |
512               |    2475.51    |   2416.65     |
1024              |    2667.57    |   2450.02     |


la taille optimale des blocs de matrice pour ma machine est szBlock = 1024. Et en effet l'espace mémoire occupé par les 3 matrices est : 3*(1024²)*8(sizeof double) = 25Mb, ce qui semble en effet être la taille limite qui puisse être contenue dans mon cache de 32Mb (Pour 2048, on obtiendrait 100Mb). 


### Bloc + OMP

On conserve une dimension de 1024 pour avoir la version optimale de la version de calcul par blocs.

 OMP_NUM | temps(n=1024)| n=4096 |
---------|--------------|--------|
  1      |   0.777329   | 58.5556|
  2      |   0.438104   | 40.9572|
  4      |   0.280389   | 40.8391|
  6      |   0.610676   | 43.8346|
  8      |   0.619447   |        |

Pour des petites matrices (n<=1024), le Speedup est de 2.77 et intervient pour 4 threads. Il est très proche de celui que l'on obtenait pour le produit matric-matrice sans la subdivision par blocs.
En revanche, pour des grandes matrices (n=4096 par exemple) le speedup est de 1.4, contrairement à la version précédente ou il était de 1.2 environ.
On gagne donc en efficacité pour les matrices de grande taille puisque c'est là que la gestion de la mémoire cache devient cruciale.

### Bitonic Parallélélisé avec thread

Bitonic séquentiel : 

Temps calcul tri sur les entiers : 0.642676
Temps calcul tri sur les vecteurs : 22.8185

Bitonic parallèle : 

Temps calcul tri sur les entiers : 0.371432
Temps calcul tri sur les vecteurs : 14.3885

On obtient un speedup de 2 pour le tri sur les entiers et un speedup de 1.6 pour le tri sur les vecteurs. Ce peut être expliqué par le fait que la parallélisation n'influe que peu sur le temps d'accès à la mémoire, or c'est celui-là qui domine pour le tri sur les vecteurs.

### Bhudda + OMP

Bhudda séquentiel : 

Temps calcul Bhudda 1 : 0.599236
Temps calcul Bhudda 2 : 0.499484
Temps calcul Bhudda 3 : 0.0583308

Mesures pour Bhudda3 : 

 OMP_NUM | temps        | speedup |
---------|--------------|---------|
  1      |   0.0583308  |    1    |
  2      |   0.0583585  |    1    |
  3      |   0.0427801  |   1.36  |
  4      |   0.0373403  |   1.56  |
  8      |   0.0440734  |   1.22  |



# Tips 

```
	env 
	OMP_NUM_THREADS=4 ./dot_product.exe
```

```
    $ for i in $(seq 1 4); do elap=$(OMP_NUM_THREADS=$i ./TestProductOmp.exe|grep "Temps CPU"|cut -d " " -f 7); echo -e "$i\t$elap"; done > timers.out
```
