from PyP100 import PyP100

# --- Tapo Device Configuration ---
TAPO_IP = "192.168.0.214"  # Replace with your Tapo device's IP address
TAPO_EMAIL = ""  # Your TP-Link Tapo account email
TAPO_PASSWORD = ""  # Your TP-Link Tapo account password


def switch_tapo_plug(turn_on=True):
    """
    Function to switch on a Tapo plug device.
    This function is called when the Flask app receives a request to control the Tapo plug.
    """
    # Ensure the required parameters are provided
    if not TAPO_IP or not TAPO_EMAIL or not TAPO_PASSWORD:
        return "Tapo device configuration is incomplete.", 400

    try:
        p100 = PyP100.P100(
            TAPO_IP, TAPO_EMAIL, TAPO_PASSWORD
        )  # Initialize the Tapo plug with IP, email, and password
        print("Initializing Tapo plug with provided credentials...")

        p100.handshake()  # Creates the cookies required for further methods
        print("Handshake successful. Cookies created.")
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
