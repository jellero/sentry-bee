import json
import os
import sqlite3
import threading
from contextlib import closing
from datetime import datetime, timezone
from typing import Any

import paho.mqtt.client as mqtt
from fastapi import FastAPI, HTTPException
from pydantic import BaseModel

DB_PATH = os.getenv("SB_DB_PATH", "/data/sentry-bee.db")
MQTT_HOST = os.getenv("SB_MQTT_HOST", "localhost")
MQTT_PORT = int(os.getenv("SB_MQTT_PORT", "1883"))
MQTT_USER = os.getenv("SB_MQTT_USER")
MQTT_PASSWORD = os.getenv("SB_MQTT_PASSWORD")

app = FastAPI(title="Sentry-Bee API", version="0.1.0")
_db_lock = threading.Lock()
_mqtt_client: mqtt.Client | None = None


def db() -> sqlite3.Connection:
    os.makedirs(os.path.dirname(DB_PATH) or ".", exist_ok=True)
    conn = sqlite3.connect(DB_PATH, check_same_thread=False)
    conn.row_factory = sqlite3.Row
    return conn


def init_db() -> None:
    with closing(db()) as conn:
        conn.executescript(
            """
            CREATE TABLE IF NOT EXISTS telemetry (
              id INTEGER PRIMARY KEY AUTOINCREMENT,
              hive_id TEXT NOT NULL,
              ts INTEGER NOT NULL,
              state TEXT NOT NULL,
              score REAL NOT NULL,
              payload TEXT NOT NULL,
              received_at TEXT NOT NULL
            );
            CREATE INDEX IF NOT EXISTS idx_telemetry_hive_ts ON telemetry(hive_id, ts DESC);

            CREATE TABLE IF NOT EXISTS events (
              id INTEGER PRIMARY KEY AUTOINCREMENT,
              event_key TEXT UNIQUE,
              hive_id TEXT NOT NULL,
              ts INTEGER NOT NULL,
              label TEXT NOT NULL DEFAULT 'unknown',
              payload TEXT NOT NULL,
              received_at TEXT NOT NULL
            );
            CREATE INDEX IF NOT EXISTS idx_events_hive_ts ON events(hive_id, ts DESC);
            """
        )
        conn.commit()


def now_iso() -> str:
    return datetime.now(timezone.utc).isoformat()


def handle_telemetry(payload: dict[str, Any]) -> None:
    hive = str(payload["hive"])
    ts = int(payload["ts"])
    state = str(payload.get("state", "unknown"))
    score = float(payload.get("score", 0.0))
    raw = json.dumps(payload, separators=(",", ":"))
    with _db_lock, closing(db()) as conn:
        conn.execute(
            "INSERT INTO telemetry(hive_id,ts,state,score,payload,received_at) VALUES(?,?,?,?,?,?)",
            (hive, ts, state, score, raw, now_iso()),
        )
        conn.commit()


def handle_event(payload: dict[str, Any]) -> None:
    hive = str(payload["hive"])
    ts = int(payload["ts"])
    event_key = str(payload.get("event_id", f"{hive}:{ts}"))
    label = str(payload.get("label", "unknown"))
    raw = json.dumps(payload, separators=(",", ":"))
    with _db_lock, closing(db()) as conn:
        conn.execute(
            "INSERT OR REPLACE INTO events(event_key,hive_id,ts,label,payload,received_at) VALUES(?,?,?,?,?,?)",
            (event_key, hive, ts, label, raw, now_iso()),
        )
        conn.commit()


def on_connect(client: mqtt.Client, userdata: Any, flags: Any, reason_code: Any, properties: Any = None) -> None:
    client.subscribe("sentry-bee/v1/+/telemetry", qos=1)
    client.subscribe("sentry-bee/v1/+/event", qos=1)


def on_message(client: mqtt.Client, userdata: Any, msg: mqtt.MQTTMessage) -> None:
    try:
        payload = json.loads(msg.payload.decode("utf-8"))
        if msg.topic.endswith("/telemetry"):
            handle_telemetry(payload)
        elif msg.topic.endswith("/event"):
            handle_event(payload)
    except (ValueError, KeyError, TypeError, UnicodeDecodeError) as exc:
        print(f"discarding invalid MQTT message topic={msg.topic}: {exc}")


def start_mqtt() -> None:
    global _mqtt_client
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id="sentry-bee-backend")
    if MQTT_USER:
        client.username_pw_set(MQTT_USER, MQTT_PASSWORD)
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect_async(MQTT_HOST, MQTT_PORT, keepalive=60)
    client.loop_start()
    _mqtt_client = client


@app.on_event("startup")
def startup() -> None:
    init_db()
    start_mqtt()


@app.on_event("shutdown")
def shutdown() -> None:
    if _mqtt_client:
        _mqtt_client.loop_stop()
        _mqtt_client.disconnect()


@app.get("/health")
def health() -> dict[str, str]:
    return {"status": "ok"}


@app.get("/api/hives")
def hives() -> list[dict[str, Any]]:
    with closing(db()) as conn:
        rows = conn.execute(
            """
            SELECT t.hive_id, t.ts, t.state, t.score, t.payload
            FROM telemetry t
            JOIN (SELECT hive_id, MAX(ts) AS ts FROM telemetry GROUP BY hive_id) m
              ON m.hive_id=t.hive_id AND m.ts=t.ts
            ORDER BY t.hive_id
            """
        ).fetchall()
    return [{**json.loads(r["payload"]), "db_state": r["state"]} for r in rows]


@app.get("/api/hives/{hive_id}/latest")
def latest(hive_id: str) -> dict[str, Any]:
    with closing(db()) as conn:
        row = conn.execute(
            "SELECT payload FROM telemetry WHERE hive_id=? ORDER BY ts DESC LIMIT 1", (hive_id,)
        ).fetchone()
    if not row:
        raise HTTPException(status_code=404, detail="hive not found")
    return json.loads(row["payload"])


@app.get("/api/hives/{hive_id}/history")
def history(hive_id: str, limit: int = 500) -> list[dict[str, Any]]:
    limit = max(1, min(limit, 5000))
    with closing(db()) as conn:
        rows = conn.execute(
            "SELECT payload FROM telemetry WHERE hive_id=? ORDER BY ts DESC LIMIT ?", (hive_id, limit)
        ).fetchall()
    return [json.loads(r["payload"]) for r in rows]


@app.get("/api/events")
def events(hive_id: str | None = None, limit: int = 200) -> list[dict[str, Any]]:
    limit = max(1, min(limit, 2000))
    with closing(db()) as conn:
        if hive_id:
            rows = conn.execute(
                "SELECT id,event_key,hive_id,ts,label,payload FROM events WHERE hive_id=? ORDER BY ts DESC LIMIT ?",
                (hive_id, limit),
            ).fetchall()
        else:
            rows = conn.execute(
                "SELECT id,event_key,hive_id,ts,label,payload FROM events ORDER BY ts DESC LIMIT ?", (limit,)
            ).fetchall()
    return [dict(r) | {"payload": json.loads(r["payload"])} for r in rows]


class EventLabel(BaseModel):
    label: str


@app.post("/api/events/{event_id}/label")
def label_event(event_id: int, req: EventLabel) -> dict[str, Any]:
    allowed = {
        "unknown", "swarm", "queenless_confirmed", "robbing", "hornet_attack",
        "inspection", "feeding", "transport", "weather", "false_positive"
    }
    if req.label not in allowed:
        raise HTTPException(status_code=400, detail=f"unsupported label; allowed={sorted(allowed)}")
    with _db_lock, closing(db()) as conn:
        cur = conn.execute("UPDATE events SET label=? WHERE id=?", (req.label, event_id))
        conn.commit()
    if cur.rowcount == 0:
        raise HTTPException(status_code=404, detail="event not found")
    return {"id": event_id, "label": req.label}
