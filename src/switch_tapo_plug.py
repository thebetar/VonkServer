import os
import sys
from dotenv import load_dotenv
from PyP100 import PyP100

# Load environment variables from .env file
load_dotenv()

print(os.environ.get("TAPO_EMAIL"))

# --- Tapo Device Configuration ---
TEMP_TAPO_IP = os.getenv("TEMP_TAPO_IP", "")
HUM_TAPO_IP = os.getenv("HUM_TAPO_IP", "")
TAPO_EMAIL = os.getenv("TAPO_EMAIL", "")
TAPO_PASSWORD = os.getenv("TAPO_PASSWORD", "")


def switch_tapo_plug(ip=TEMP_TAPO_IP, turn_on=True):
    """
    Function to switch on a Tapo plug device.
    This function is called when the Flask app receives a request to control the Tapo plug.
    """
    # Ensure the required parameters are provided
    if not ip or not TAPO_EMAIL or not TAPO_PASSWORD:
        return "Tapo device configuration is incomplete.", 400

    try:
        p100 = PyP100.P100(
            ip, TAPO_EMAIL, TAPO_PASSWORD
        )  # Initialize the Tapo plug with IP, email, and password
        p100.handshake()  # Creates the cookies required for further methods
        p100.login()  # Sends credentials to the plug and creates AES Key and IV for further methods

        if turn_on:
            p100.turnOn()
        else:
            p100.turnOff()

        p100.getDeviceInfo()  # Returns dict with all the device info of the connected plug
        p100.getDeviceName()  # Returns the name of the connected plug set in the app

        return "Tapo plug switched successfully."
    except Exception as e:
        print(f"An error occurred: {e}")
        return None


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python switch_tapo_plug.py <temp/hum> <on/off>")
        sys.exit(1)

    type = sys.argv[1].lower()
    action = sys.argv[2].lower()

    if type == "temp":
        tapo_ip = TEMP_TAPO_IP
    elif type == "hum":
        tapo_ip = HUM_TAPO_IP
    else:
        print("Invalid type. Use 'temp' or 'hum'.")
        sys.exit(1)

    if action == "on":
        result = switch_tapo_plug(ip=tapo_ip, turn_on=True)
    elif action == "off":
        result = switch_tapo_plug(ip=tapo_ip, turn_on=False)
    else:
        print("Invalid argument. Use 'on' or 'off'.")
        sys.exit(1)

    print(result)
