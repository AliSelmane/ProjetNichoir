import paho.mqtt.client as mqtt
import time
import os

# Configuration MQTT (Assurez-vous qu'elle corresponde au code Arduino !)
MQTT_BROKER = "VOTRE_BROKER_MQTT" # Ex: "test.mosquitto.org"
MQTT_PORT = 1883
TOPIC_IMAGE = "camera/image/data" # Sujet pour les données binaires
TOPIC_INFO = "camera/image/info"  # Sujet pour les marqueurs START/END

# --- Variables de l'image ---
receiving_image = False  # Indicateur qu'une transmission est en cours
image_data = bytearray() # Buffer pour assembler les fragments
expected_size = 0        # Taille totale attendue de l'image
image_counter = 0        # Compteur pour nommer les fichiers

def on_connect(client, userdata, flags, rc):
    """Callback : Exécuté lorsque le client se connecte au broker."""
    print("Connecté au broker MQTT avec le code : " + str(rc))
    
    # S'abonne aux deux sujets
    client.subscribe(TOPIC_INFO)
    client.subscribe(TOPIC_IMAGE)

def on_message(client, userdata, msg):
    """Callback : Exécuté à la réception de chaque message."""
    global receiving_image, image_data, expected_size, image_counter

    if msg.topic == TOPIC_INFO:
        # --- Gestion des messages d'information (Début/Fin) ---
        payload_str = msg.payload.decode()
        
        if payload_str.startswith("START:"):
            # Début de la transmission
            try:
                expected_size = int(payload_str.split(":")[1])
                image_data = bytearray() # Réinitialise le buffer
                receiving_image = True
                print(f"--- Nouvelle image. Taille attendue : {expected_size} octets ---")
            except ValueError:
                print("Erreur: Le message START n'a pas la bonne taille.")
            
        elif payload_str == "END" and receiving_image:
            # Fin de la transmission
            receiving_image = False
            
            # Vérification de la taille finale pour s'assurer qu'aucune donnée n'a été perdue
            if len(image_data) == expected_size and len(image_data) > 0:
                image_counter += 1
                filename = f"capture_{image_counter}_{int(time.time())}.jpg"
                
                # Sauvegarde de l'image
                try:
                    with open(filename, "wb") as f:
                        f.write(image_data)
                    print(f"✅ Image enregistrée : {filename} (Taille réelle: {len(image_data)} octets)")
                except Exception as e:
                    print(f"Erreur lors de l'enregistrement du fichier : {e}")

            else:
                print(f"❌ Erreur : Taille reçue ({len(image_data)}) ne correspond pas à la taille attendue ({expected_size}). Image ignorée.")
                if len(image_data) > 0:
                     # Sauvegarder les données partiellement reçues à des fins de débogage
                    with open("debug_partial.bin", "wb") as f:
                        f.write(image_data)


    elif msg.topic == TOPIC_IMAGE and receiving_image:
        # --- Réception des paquets de données binaires ---
        image_data.extend(msg.payload)
        # print(f"Fragment reçu. Taille actuelle du buffer : {len(image_data)} octets.")

# --- Boucle principale pour lancer le client ---
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

print(f"Tentative de connexion au broker {MQTT_BROKER}:{MQTT_PORT}...")
try:
    client.connect(MQTT_BROKER, MQTT_PORT, 60)
except Exception as e:
    print(f"Impossible de se connecter au broker MQTT. Vérifiez l'adresse et le port. Erreur: {e}")
    exit()

# Cette boucle maintient la connexion et traite les messages entrants
client.loop_forever()