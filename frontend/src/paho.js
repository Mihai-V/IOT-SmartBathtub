let client = null;

export const getClient = () => {
    return new Promise((resolve, _) => {
        if(client) {
            return resolve(client);
        }
        // Load the Paho script
        let script = document.createElement('script');
        script.src = 'https://cdnjs.cloudflare.com/ajax/libs/paho-mqtt/1.0.1/mqttws31.js';
        document.head.appendChild(script);
        script.onload = () => {
            client = new window.Paho.MQTT.Client(process.env.REACT_APP_MQTT_HOST,
                                    parseInt(process.env.REACT_APP_MQTT_PORT),
                                    process.env.REACT_APP_MQTT_CLIENT_ID);
            resolve(client);
        }
    });
}