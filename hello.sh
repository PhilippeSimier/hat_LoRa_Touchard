#!/bin/bash
# Pour IPv4 wifi lan
ip_address=""

while [ -z "$ip_address" ]; do
    # Essaye de récupérer l'adresse IP
    ip_address=$(ip -4 addr show wlan0 | grep -oP '(?<=inet\s)\d+(\.\d+){3}')

    # Si adresse IP est encore vide, attends un peu
    if [ -z "$ip_address" ]; then
        /opt/oled $HOSTNAME boot
        sleep 5  # Attendre 5 secondes avant de réessayer
    fi
done


# Afficher l'adresse IP
echo $HOSTNAME $ip_address
#echo $ip_address
/opt/oled $HOSTNAME $ip_address
/opt/stop &
