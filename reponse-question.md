# Projet ESP32 – FreeRTOS, MQTT, LittleFS et Supervision

## Présentation du projet

Ce projet consiste à développer un système embarqué basé sur un **ESP32** utilisant **FreeRTOS** afin de gérer plusieurs tâches en parallèle.
Le système permet l’acquisition de mesures via des capteurs, la communication avec un broker **MQTT**, le stockage local des données en cas de panne réseau avec **LittleFS**, ainsi que l’affichage et la supervision des données via des outils comme **Node-RED**, **InfluxDB** et **Grafana**.

L’objectif principal est d’assurer un fonctionnement fiable, même en cas de coupure WiFi ou d’indisponibilité temporaire du broker MQTT.

---

## Q1 – Fonction, tâche, interruption et timer

Une **fonction** est un bloc de code exécuté lorsqu’il est appelé.

Une **tâche FreeRTOS** est exécutée indépendamment par l’ordonnanceur. Elle permet de réaliser plusieurs traitements en parallèle, comme l’acquisition des capteurs, la communication MQTT ou la gestion de l’interface Web.

Une **interruption** permet de réagir immédiatement à un événement externe, sans attendre l’exécution normale du programme.

Un **timer** permet d’exécuter automatiquement une action à intervalle régulier.

Dans notre projet, les tâches FreeRTOS permettent de séparer clairement l’acquisition des capteurs, la communication MQTT et l’interface Web.

---

## Q2 – Justification des priorités

La tâche **TaskSensors** possède la priorité la plus élevée car l’acquisition des mesures est la fonction principale du système.

La tâche **TaskMQTT** possède une priorité intermédiaire, car les données peuvent être temporairement stockées dans une **Queue** avant d’être envoyées.

Les tâches liées au **Web** et à l’**affichage** ont une priorité plus faible, car elles ne doivent pas perturber les mesures des capteurs.

Cette organisation permet de garantir que les fonctions critiques du système sont exécutées en priorité.

---

## Q3 – Queue, Mutex et Sémaphore

Une **Queue** est utilisée pour transmettre des mesures entre les tâches sans accéder directement aux variables partagées.

Un **Mutex** permet de protéger les données partagées contre les accès simultanés entre plusieurs tâches.

Un **Sémaphore** sert à signaler un événement, comme une alarme, une détection PIR ou une action déclenchée par une interruption.

Dans notre projet, ces mécanismes permettent d’améliorer la fiabilité et d’éviter les conflits entre les tâches.

---

## Q4 – Boucle MQTT infinie

Une boucle qui publie des messages MQTT en permanence saturerait le processeur et le réseau.

Cela pourrait ralentir ou bloquer les autres tâches, notamment l’acquisition des capteurs.

C’est pourquoi les publications MQTT sont contrôlées dans le temps à l’aide de FreeRTOS, par exemple avec `vTaskDelay()` ou `vTaskDelayUntil()`.

---

## Q5 – Utilisation de `xQueueSend()` avec `portMAX_DELAY`

L’avantage de `xQueueSend()` avec `portMAX_DELAY` est qu’aucune donnée n’est perdue immédiatement, car la tâche attend qu’une place soit disponible dans la Queue.

Cependant, l’inconvénient est que la tâche peut rester bloquée longtemps si la Queue est pleine.

Dans notre projet, cela pourrait empêcher l’acquisition de nouvelles mesures si la tâche capteur reste bloquée trop longtemps.

---

## Q6 – Partage de la température entre plusieurs tâches

Pour partager la température entre plusieurs tâches, il est possible d’utiliser un **Mutex** ou une **Queue**.

Dans notre projet, la **Queue** est privilégiée car elle permet de transmettre les données proprement entre les tâches, sans accès concurrent direct à une même variable.

Cette méthode améliore la robustesse du système.

---

## Q7 – Différence entre `vTaskDelay()` et `vTaskDelayUntil()`

La fonction `vTaskDelay()` met une tâche en pause pendant une durée donnée.

La fonction `vTaskDelayUntil()` permet de conserver une période d’exécution régulière et précise.

Pour les mesures capteurs, `vTaskDelayUntil()` est plus adaptée, car elle garantit une fréquence d’échantillonnage constante.

---

## Q8 – Coupure WiFi de 45 minutes

En cas de coupure WiFi de 45 minutes, les capteurs et les actionneurs continuent de fonctionner normalement.

Les données sont stockées localement dans **LittleFS** afin d’éviter leur perte.

Lorsque le réseau revient, les mesures stockées sont automatiquement retransmises vers le broker MQTT.

---

## Q9 – Retour du broker MQTT

Lorsque le broker MQTT redevient disponible, les données stockées localement sont publiées dans l’ordre **FIFO**.

Cela permet de conserver la chronologie des mesures.

Grâce au stockage local avec LittleFS, le risque de perte de données est très faible.

---

## Q10 – LittleFS plein

Si la mémoire **LittleFS** est pleine, les données les plus anciennes peuvent être supprimées afin de conserver les mesures les plus récentes.

Une alerte peut également être générée pour signaler le problème.

Cette stratégie permet au système de continuer à fonctionner même avec une mémoire limitée.

---

## Q11 – Trois défauts du projet

Le projet présente quelques limites :

1. Il ne possède pas d’horloge **RTC**, ce qui peut compliquer l’horodatage précis des mesures en cas de coupure réseau.
2. Le stockage **LittleFS** reste limité en capacité.
3. L’authentification est simple et pourrait être renforcée.

Ces points pourraient être améliorés avec un module RTC, une carte SD et un système de communication sécurisé en HTTPS.

---

## Q12 – Trois améliorations possibles

Avec plus de temps, plusieurs améliorations pourraient être ajoutées :

1. L’intégration de **Grafana** pour une visualisation avancée des données.
2. L’ajout de notifications **Telegram** en cas d’alerte.
3. La mise en place d’un monitoring détaillé du CPU sur les deux cœurs de l’ESP32.

Ces améliorations rendraient le système plus complet, plus pratique et plus facile à surveiller.

---

## Q13 – Point faible principal

Le point faible principal du projet reste le **réseau**.

Une panne WiFi empêche la communication MQTT ainsi que l’accès à l’interface Web.

Cependant, le système continue à fonctionner localement grâce à FreeRTOS et au stockage LittleFS.

---

## Q14 – Retour ignoré de `xQueueSend()`

Si le retour de `xQueueSend()` n’est pas vérifié, une Queue pleine peut provoquer la perte d’une mesure.

Il est donc préférable de tester systématiquement le résultat de l’envoi.

Cela permet de détecter les erreurs et de mettre en place une solution, comme un stockage temporaire dans LittleFS ou une alerte.

---

## Q15 – Parcours complet d’une mesure

Une mesure est d’abord lue par le capteur **DHT22**.

Elle est ensuite traitée par la tâche **TaskSensors**, puis envoyée dans une **Queue FreeRTOS**.

La tâche **TaskMQTT** récupère ensuite la mesure et la publie vers le broker MQTT.

Les données sont ensuite reçues par **Node-RED**, stockées dans **InfluxDB**, puis affichées dans **Grafana**.

En cas de panne réseau, les données sont conservées dans **LittleFS** jusqu’à la reconnexion.

---

## Conclusion

Ce projet montre l’intérêt de FreeRTOS dans un système embarqué basé sur ESP32.

Grâce aux tâches, aux Queues, aux Mutex, aux Sémaphores et au stockage local, le système reste fiable même en cas de problème réseau.

L’architecture choisie permet de séparer les responsabilités, d’améliorer la stabilité du programme et de garantir une meilleure gestion des mesures capteurs.
