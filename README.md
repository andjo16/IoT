# Making PF Ceramics Club smart


The ardiuno code for the LoRa end-devices is given as
- LightSwitch.ino
- PeopleCounter.ino
- VentilationControl.ino

The following library has been used for the ardiuno programs
- Servo control: https://github.com/RoboticsBrno/ServoESP32  
  Has to be installed from zip file
- LoRaWAN communication: MCCI LoRaWAN LMIC (V3.3.0)  
	Can be found in ardiuno library manager  
	Also available here: https://www.arduino.cc/reference/en/libraries/mcci-lorawan-lmic-library/
- Thermocouple module MAX31855: Adafruit MAX31855 Library (V1.3.0)  
	Can be found in arduino library manager

### Backend
The backend uses the following libraries:
 - fastapi
 - uvicorn
 - asyncio-mqtt
 - websockets
 - SQLAlchemy
 - psycopg2

To run it, Python 3 is needed. To run locally, execute the following commands:
```
python -m venv venv
source venv/bin/activate
pip install -r requirements.txt
uvicorn main::app
```
	
### Web page
Uses `plotly` library, which is include in the code. To view the web page, open `web_page/index.html` with any browser.

### Mobile app
Uses the `Flutter` framework, and the `http` Flutter library, which is installed automatically upon building the app.
To build the app, install `flutter` and execute:
```
flutter build apk
```
