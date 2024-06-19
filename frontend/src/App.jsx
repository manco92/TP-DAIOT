import { useEffect, useState } from "react";
import { Card } from "./components/Card";

function App() {
  const [devices, setDevices] = useState([]);
  const [selectedDevice, setSelectedDevice] = useState(null);
  const [deviceLogs, setDeviceLogs] = useState([]);

  useEffect(() => {
    const getDevices = async () => {
      try {
        const response = await fetch("http://localhost:3000/dispositivos");
        const data = await response.json();
        setDevices(data.data);
      } catch (error) {
        console.error("Error fetching devices");
      }
    };
    getDevices();
  }, []);

  useEffect(() => {
    if (selectedDevice) {
      const getLogs = async () => {
        try {
          const response = await fetch(
            `http://localhost:3000/logs/${selectedDevice.dispositivoId}`
          );
          const data = await response.json();
          setDeviceLogs(data.data);
        } catch (error) {
          console.error(
            `Error fetching device logs ${selectedDevice.dispositivoId}`
          );
        }
      };
      getLogs();
    }
  }, [selectedDevice]);

  return (
    <main>
      <h1>Dispositivos</h1>
      <div style={{ display: "flex", gap: 16 }}>
        {devices.map((device) => (
          <Card key={device._id} onClick={() => setSelectedDevice(device)}>
            {device.ubicacion}
          </Card>
        ))}
      </div>
      {/* {selectedDevice && <div>{JSON.stringify(selectedDevice, null, 2)}</div>} */}
      <hr />
      {selectedDevice &&
        (deviceLogs.length
          ? deviceLogs.map((item) => {
              const ts = new Date(item.ts);
              return (
                <div key={item._id}>{`Log ID: ${
                  item.logId
                } - Fecha: ${ts.toLocaleDateString("es-AR", {
                  weekday: "long",
                  year: "numeric",
                  month: "long",
                  day: "numeric",
                })} ${
                  " - " +
                  ts.getHours() +
                  ":" +
                  ts.getMinutes() +
                  ":" +
                  ((ts.getSeconds() < 10 ? "0" : "") + ts.getSeconds())
                } -  Humedad: ${Number(item.ehumedad).toFixed(
                  2
                )} - Temperatura: ${Number(item.etemperatura).toFixed(
                  2
                )}`}</div>
              );
            })
          : "No logs")}
      ;
    </main>
  );
}

export default App;
