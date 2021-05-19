import 'dart:convert';
import 'dart:async';

import 'package:flutter/material.dart';
import 'package:http/http.dart' as http;

const String BACKEND_URL = "https://dt-iotgroup6-backend.herokuapp.com";
const double FONT_SIZE = 30;

void main() {
  runApp(MaterialApp(
    title: 'iot_frontend',
    theme: ThemeData(
      primarySwatch: Colors.blue,
    ),
    home: HomePage(),
  ));
}

class HomePage extends StatefulWidget {
  @override
  _HomePageState createState() => _HomePageState();
}

class _HomePageState extends State<HomePage> {
  bool workshopLightState = false;
  bool loungeLightState = false;
  bool ventilationState = false;
  int numberPeople = 0;
  Timer timer;
  Widget divider = Divider(
    color: Colors.grey,
    height: 20,
    thickness: 2,
    indent: 30,
    endIndent: 30,
  );

  Future<void> getNumberPeople() async {
    String url = '${BACKEND_URL}/number_people';
    var res = await http.get(Uri.parse(url));

    var jsonData = json.decode(res.body);
    setState(() {
      numberPeople = jsonData["number_people"];
    });
  }

  Future<void> getLight() async {
    String url = '${BACKEND_URL}/lights';
    var res = await http.get(Uri.parse(url));

    var jsonData = json.decode(res.body);
    setState(() {
      workshopLightState = jsonData["state_1"];
      loungeLightState = jsonData["state_2"];
    });
  }

  Future<void> getVentilation() async {
    String url = '${BACKEND_URL}/ventilation';
    var res = await http.get(Uri.parse(url));

    var jsonData = json.decode(res.body);
    setState(() {
      ventilationState = jsonData["state"];
    });
  }

  Future<void> setSensorState(int sensorId, bool state) async {
    String url = '${BACKEND_URL}/cibicom_downlink';
    await http.post(
      Uri.parse(url),
      body: json.encode({'sensor_id': sensorId, 'state': state}),
    );
  }

  void renderData() {
    getNumberPeople();
    getLight();
    getVentilation();
  }

  Widget stateTextWidget(bool state) {
    if (state) {
      return Text(
        'On',
        style: TextStyle(
          fontSize: FONT_SIZE,
          color: Colors.green,
        ),
      );
    } else {
      return Text(
        'Off',
        style: TextStyle(
          fontSize: FONT_SIZE,
          color: Colors.red,
        ),
      );
    }
  }

  Widget stateButtonWidget(bool state, int sensorId) {
    String text;
    bool buttonState;
    if (state) {
      text = "Turn off";
      buttonState = false;
    } else {
      text = "Turn on";
      buttonState = true;
    }
    return RaisedButton(
            child: Text(
              text,
              style: TextStyle(
                fontSize: FONT_SIZE/2.0,
              ),
            ),
            onPressed: () {
              setSensorState(sensorId, buttonState);
            },
          );
  }

  @override
  void initState() {
    renderData();
    timer = Timer.periodic(Duration(seconds: 5), (Timer t) => renderData());
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text('IoT App'),
        centerTitle: true,
      ),
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: <Widget>[
            Text(
              'Number of people',
              style: TextStyle(
                fontSize: FONT_SIZE,
              ),
            ),
            Text(
              '${numberPeople}',
              style: TextStyle(
                fontSize: FONT_SIZE,
              ),
            ),
            divider,


            Text(
              'Workshop light',
              style: TextStyle(
                fontSize: FONT_SIZE,
              ),
            ),
            stateTextWidget(workshopLightState),
            stateButtonWidget(workshopLightState, 5),
            divider,


            Text(
              'Lounge light',
              style: TextStyle(
                fontSize: FONT_SIZE,
              ),
            ),
            stateTextWidget(loungeLightState),
            stateButtonWidget(loungeLightState, 6),
            divider,


            Text(
              'Ventilation',
              style: TextStyle(
                fontSize: FONT_SIZE,
              ),
            ),
            stateTextWidget(ventilationState),
            stateButtonWidget(ventilationState, 3),
          ],
        ),
      ),
    );
  }
}
