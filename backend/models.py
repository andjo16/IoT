""" Models
This module contains the definition of the models that the API uses for
sending and receiving data. The API uses json format.
"""
from typing import List

from pydantic import BaseModel


class Temperature(BaseModel):
    temperature_1: List[float]
    timestamp_1: List[int]
    temperature_2: List[float]
    timestamp_2: List[int]


class Lights(BaseModel):
    state_1: bool
    timestamp_1: int
    state_2: bool
    timestamp_2: int


class Ventilation(BaseModel):
    state: bool
    timestamp: int


class NumberPeople(BaseModel):
    number_people: int
    timestamp: int


class CibicomData(BaseModel):
    cmd: str
    EUI: str
    ts: int
    ack: bool
    bat: int
    fcnt: int
    port: int
    data: str


class CibicomDownlink(BaseModel):
    sensor_id: int
    state: bool
