# TP2 de ROLLAND Tanguy

`pandoc -s --toc tp2.md --css=./github-pandoc.css -o tp2.html`





## Mandelbrot 

*Expliquer votre stratégie pour faire une partition équitable des lignes de l'image entre chaque processus*

           | Taille image : 800 x 600 | 
-----------+---------------------------
séquentiel |              
1          |              
2          |              
3          |              
4          |              
8          |              

Partition des lignes : Il suffit de diviser le nombre total de lignes par le nombre de processus. Chaque processus entre le rang 0 et le rang nbp-2 (inclus) traite donc H/nbp lignes. Le dernier processus (rang nbp-1) traitera H/nbp + H%nbp lignes, on s'assure ainsi que toutes les lignes sont traitées même si H n'est pas divisible par nbp.

  nbp  |   t
------------------ 
   1   | 122.37
  30   | 116.82
  45   | 126.08
  50   | 111.28
  55   | 109.54
  60   | 114.45
  70   | 109.52
 120   | 115.60
 300   | 120.51

Le temps d'éxécution en séquentiel vaut : ts = 120.55
le minimum du temps d'éxécution en parallèle se trouve aux environs de nbp = 55 et vaut environ 109.5
On a donc un speedup maximal : Smax = 1.1

Aurtrement dit l'accélération fournie par le parallélisme est très faible.
En effet, certaines zones de l'image sont beaucoup plus longues à traiter que d'autres du fait des valeurs des pixels. Ainsi, une répartition des lignes par zone d'image n'améliore que très peu le temps d'éxécution.
  
*Discuter sur ce qu'on observe, la logique qui s'y cache.*

*Expliquer votre stratégie pour faire une partition dynamique des lignes de l'image entre chaque processus*

On peut faire un système de partition 

           | Taille image : 800 x 600 | 
-----------+---------------------------
séquentiel |              
1          |              
2          |              
3          |              
4          |              
8          |              



## Produit matrice-vecteur



*Expliquer la façon dont vous avez calculé la dimension locale sur chaque processus, en particulier quand le nombre de processus ne divise pas la dimension de la matrice.*

On isole chaque somme partielle du produit matrice*vecteur par colonnes et on les somme grâce à MPI_Allreduce.
