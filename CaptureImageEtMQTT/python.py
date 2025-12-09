import paho.mqtt.client as mqtt
import os
import sys

# ==========================================
# CONFIGURATION
# ==========================================
# Doit être le même que dans l'ESP32
MQTT_BROKER = "test.mosquitto.org" 
MQTT_PORT = 1883
TOPIC_IMAGE = "camera/image/data" 
TOPIC_INFO = "camera/image/info"  
SAVE_FOLDER = "photos_nichoir"    # Dossier où seront stockées les photos

# ==========================================
# VARIABLES GLOBALES
# ==========================================
receiving_image = False 
image_data = bytearray() 
expected_size = 0        
current_filename = ""    

# Création du dossier si inexistant
if not os.path.exists(SAVE_FOLDER):
    try:
        os.makedirs(SAVE_FOLDER)
        print(f"Dossier '{SAVE_FOLDER}' créé.")
    except OSError as e:
        print(f"Erreur création dossier: {e}")

def on_connect(client, userdata, flags, rc):
    """Callback : Exécuté lorsque le client se connecte au broker."""
    if rc == 0:
        print(f"✅ Connecté au broker MQTT ({MQTT_BROKER})")
        # S'abonne aux deux sujets
        client.subscribe([(TOPIC_INFO, 0), (TOPIC_IMAGE, 0)])
        print("   En attente de photos...")
    else:
        print(f"❌ Échec connexion, code retour : {rc}")

def on_message(client, userdata, msg):
    """Callback : Exécuté à la réception de chaque message."""
    global receiving_image, image_data, expected_size, current_filename

    # --- CAS 1 : MESSAGE D'INFORMATION (START / END) ---
    if msg.topic == TOPIC_INFO:
        try:
            payload_str = msg.payload.decode("utf-8")
        except:
            print("Erreur décodage message info")
            return
        
        # > DÉBUT DE TRANSMISSION
        if payload_str.startswith("START:"):
            try:
                # Format attendu: START:Taille:ID:Lieu:Index
                parts = payload_str.split(":")
                
                expected_size = int(parts[1])
                dev_id = parts[2]
                loc = parts[3]
                idx = parts[4]
                
                # On construit un nom de fichier propre
                current_filename = f"{dev_id}_{loc}_{idx}.jpg"
                
                # Réinitialisation
                image_data = bytearray() 
                receiving_image = True
                print(f"\n📥 Réception : {current_filename} ({expected_size} octets)...")
                
            except IndexError:
                print("⚠️ Erreur format Header (START malformé)")
            except ValueError:
                print("⚠️ Erreur format Taille (Pas un nombre)")
            
        # > FIN DE TRANSMISSION
        elif payload_str == "END" and receiving_image:
            receiving_image = False
            received_len = len(image_data)
            
            # On accepte une petite marge d'erreur ou on exige la taille exacte
            # Ici on vérifie simplement qu'on a reçu des données
            if received_len > 0:
                full_path = os.path.join(SAVE_FOLDER, current_filename)
                
                try:
                    with open(full_path, "wb") as f:
                        f.write(image_data)
                    print(f"✅ Image enregistrée : {full_path} ({received_len}/{expected_size} octets)")
                except Exception as e:
                    print(f"❌ Erreur sauvegarde fichier : {e}")
            else:
                print("❌ Erreur : Buffer vide, image ignorée.")

    # --- CAS 2 : DONNÉES BINAIRES (L'IMAGE ELLE-MÊME) ---
    elif msg.topic == TOPIC_IMAGE and receiving_image:
        image_data.extend(msg.payload)
        # Affiche un petit point pour montrer que ça télécharge (optionnel)
        sys.stdout.write(".") 
        sys.stdout.flush()

# ==========================================
# LANCEMENT
# ==========================================
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

print(f"Tentative de connexion à {MQTT_BROKER}...")

try:
    client.connect(MQTT_BROKER, MQTT_PORT, 60)
    client.loop_forever()
except KeyboardInterrupt:
    print("\nArrêt du script.")
except Exception as e:
    print(f"Erreur critique : {e}")