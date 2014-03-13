qt-opengl
=========

Voici les codes sources des exemples abordés dans le tutoriel http://guillaume.belz.free.fr/doku.php#articles_opengl. Pour les tester, il suffit d'ouvrir le fichier .pro avec Qt Creator et de lancer l'application. Il est intéressant aussi d'étudier ces codes sources pour bien comprendre le fonctionnement et l'implémentation d'OpenGL/OpenCL dans Qt.

Application minimale QtOpenGL
=========

Cette application permet de présenter le code minimal nécessaire pour initialiser un contexte OpenGL avec Qt. Elle affiche un simple repère orthonormé et le nombre d'images par seconde (FPS). Le bouton droit de la souris permet de tourner autour du repère et la molette permet de s'approcher et de s'éloigner.

Comparaison entre les différents modes de transfert de données au GPU
=========

Cette application permet de présenter les différentes méthodes de transfert des données entre le CPU et le GPU (directe, vertex array, vertex buffer, indices). Elle affiche une heightmap avec texture. Le bouton droit de la souris permet de tourner autour du repère et la molette permet de s'approcher et de s'éloigner. La touche “T” permet d'afficher ou non la texture. La touche “F” permet d'afficher la heightmap sous forme de fil de fer ou non. La barre d'espace permet de changer de mode de transfert. Les flèches haut et bas permettent de modifier le délai d'affichage entre deux images. Pour déterminer le taux de FPS maximal, il suffit de mettre le délai à 0.

Utilisation des shaders
=========

Cette application permet de présenter l'utilisation des shaders avec Qt. Elle affiche une heightmap avec texture et ombrage de Phong. La source de lumière est de type parallèle, c'est-à-dire que la source de lumière est à l'infini. L'angle d'incidence varie au cours du temps mais il est possible de stopper l'animation en appuyant sur la touche “L”. Le bouton droit de la souris permet de tourner autour du repère et la molette permet de s'approcher et de s'éloigner. La barre d'espace permet d'afficher ou non le texte et les images 2D. Les flèches droite et gauche permettent de contrôler l'intensité de la texture. Les flèches haut et bas permettent de contrôler l'intensité des ombrages.

Rendu off-screen
=========

Cette application permet de tester différents modes de rendu off-screeen. Elle affiche une heightmap avec texture et ombrage de Phong. Le bouton droit de la souris permet de tourner autour du repère et la molette permet de s'approcher et de s'éloigner. La barre d'espace permet de changer de mode de rendu off-screen.




