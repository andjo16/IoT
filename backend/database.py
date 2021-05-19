""" Database
This module contains the helper functions to interact with the
PostgreSQL database (storing, updating and reading operations).
"""
import time

from sqlalchemy import create_engine

DB_URI = "postgresql://eeazmaghjljqly:e9d2493f339fc263a128c593313a4195b431e287b60d62fc376080fe3a272cac@ec2-34-252-251-16.eu-west-1.compute.amazonaws.com:5432/d3npo4rq9c6rh8"

engine = create_engine(DB_URI)


def insert_temperature(sensor_id: int, temperature: int):
    engine.execute("INSERT INTO temperature(sensor_id, temperature, timestamp) "
        + f"VALUES({sensor_id}, {temperature}, {int(time.time())})")

def get_temperature(sensor_id: int, limit: 30):
    return engine.execute("SELECT temperature, timestamp FROM temperature "
        + f"WHERE sensor_id={sensor_id} ORDER BY timestamp DESC "
        + f"LIMIT {limit}").all()


def update_state(sensor_id: int, state: int):
    engine.execute(f"UPDATE states SET value={state}, "
        + f"last_updated={int(time.time())} WHERE sensor_id={sensor_id}")

def get_state(sensor_id: int):
    return engine.execute("SELECT value, last_updated FROM states "
            + f"WHERE sensor_id={sensor_id}").fetchone()


