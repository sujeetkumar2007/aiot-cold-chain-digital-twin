
from pathlib import Path

import joblib
import numpy as np
from sklearn.ensemble import IsolationForest

MODEL_PATH = Path(__file__).resolve().parent / "anomaly_model.joblib"


def train_model():
    rng = np.random.default_rng(42)

    # Synthetic normal cold-storage readings
    temperature = rng.normal(5.0, 1.5, 1500)
    humidity = rng.normal(75.0, 8.0, 1500)

    X = np.column_stack((temperature, humidity))

    model = IsolationForest(
        n_estimators=200,
        contamination=0.05,
        random_state=42
    )

    model.fit(X)

    MODEL_PATH.parent.mkdir(parents=True, exist_ok=True)

    joblib.dump(
        {
            "model": model,
            "features": ["temperature", "humidity"],
            "training_profile": "Synthetic baseline: 5°C, 75% RH"
        },
        MODEL_PATH
    )

    print("Anomaly detection model trained successfully!")
    print(f"Model saved at: {MODEL_PATH}")

    return model


if __name__ == "__main__":
    train_model()
