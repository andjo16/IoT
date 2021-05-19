import time
import json
import struct
import asyncio
from typing import List
import random

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
import websockets

from models import *
import database


class SensorId:
    """ Header id of the peripheral devices. """
    TEMP_1 = 1
    TEMP_2 = 2
    VENTILATION = 3
    NUMBER_PEOPLE = 4
    LIGHT_1 = 5
    LIGHT_2 = 6

def get_eui(sensor_id: int) -> str:
    """ Returns the EUI of devices with actuators, for downlink messages. """
    if sensor_id == SensorId.VENTILATION:
        return "ABCDEF0123456789"
    elif sensor_id == SensorId.LIGHT_1:
        return "0123456789ABCDEF"
    elif sensor_id == SensorId.LIGHT_2:
        return "1357913579ABCDEF"
    else:
        return ""

# Cibicom
CBC_WEBSOCKET = "wss://iotnet.cibicom.dk/app?token=vnoTtQAAABFpb3RuZXQuY2liaWNvbS5kayM48uuVy4swkTP2zSB-F9Y="


app = FastAPI()
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

@app.get("/temperature")
async def temperature_get():
    """ GET last temperature values. """
    temp1 = []
    ts1 = []
    for temp, ts in database.get_temperature(SensorId.TEMP_1, 30):
        temp1.append(round(temp/100.0, 2))
        ts1.append(ts)
    temp2 = []
    ts2 = []
    for temp, ts in database.get_temperature(SensorId.TEMP_2, 30):
        temp2.append(round(temp/100.0, 2))
        ts2.append(ts)
    return Temperature(temperature_1 = temp1, timestamp_1 = ts1,
        temperature_2 = temp2, timestamp_2 = ts2)

@app.get("/ventilation")
async def ventilations_get():
    """ GET ventilation status. """
    v, ts = database.get_state(SensorId.VENTILATION)
    return Ventilation(state=v, timestamp=ts)
        
@app.get("/number_people")
async def number_people_get():
    """ GET current number of people. """
    data = database.get_state(SensorId.NUMBER_PEOPLE)
    return NumberPeople(number_people=data[0], timestamp=data[1])

@app.get("/lights")
async def lights_get():
    """ GET the state of both lights. """
    l1, ts1 = database.get_state(SensorId.LIGHT_1)
    l2, ts2 = database.get_state(SensorId.LIGHT_2)
    return Lights(state_1 = l1, timestamp_1 = ts1, state_2 = l2, timestamp_2=ts2)

@app.post("/cibicom_data")
async def cibicom_data_post(data: CibicomData):
    """ Receive and parse data from CIBICOM. """
    raw = bytes.fromhex(data.data)
    print(data.data)
    if data.cmd != "rx":
        return
    if raw[0] == 1:
        id1, temp1, id2, temp2, id3, vent = struct.unpack("<BhBhBB", raw)
        database.insert_temperature(id1, temp1)
        database.insert_temperature(id2, temp2)
        database.update_state(id3, vent)
    elif raw[0] == 4:
        id_sensor, number_people = struct.unpack("<BH", raw)
        database.update_state(id_sensor, number_people)
        if number_people == 0:
            state, _ = database.get_state(SensorId.LIGHT_1)
            if state:
                await set_sensor_state(SensorId.LIGHT_1, False)
            state, _ = database.get_state(SensorId.LIGHT_2)
            if state:
                await set_sensor_state(SensorId.LIGHT_2, False)
    elif raw[0] == 5:
        id_sensor, light = struct.unpack("<BB", raw)
        database.update_state(id_sensor, light)
    elif raw[0] == 6:
        id_sensor, light = struct.unpack("<BB", raw)
        database.update_state(id_sensor, light)

@app.post("/cibicom_downlink")
async def cibicom_downlink(downlink: CibicomDownlink):
    """ Send downlink messages. """
    await set_sensor_state(downlink.sensor_id, downlink.state)

async def set_sensor_state(sensor_id: int, state: bool):
    """ Helper function to connect to CIBICOM using a WebSocket. """
    eui = get_eui(sensor_id)
    if eui:
        data = f"{sensor_id:02x}{state:02x}"
        print("Data from sensor:", data)
        downlink_json = {
            "cmd": "tx",
            "EUI": eui,
            "port": 1,
            "confirmed": True,
            "data": data,
        }
        async with websockets.connect(CBC_WEBSOCKET) as websocket:
            await websocket.send(json.dumps(downlink_json))
            response = await websocket.recv()
            print("Websocket response:", response)
