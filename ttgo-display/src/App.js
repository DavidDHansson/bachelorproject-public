import './App.css';
import { useState, useEffect } from 'react';
import { Howl  } from 'howler';
import successSound from './sounds/success.mp3';
import errorSound from './sounds/error.mp3';
import mqtt from 'mqtt';

function App() {
  const [client, setClient] = useState(null)
  const [isSubed, setIsSub] = useState(false)
  const [payload, setPayload] = useState(null)
  const [connectStatus, setConnectStatus] = useState('Connect')

  const [backgroundColor, setBackgroundColor] = useState("#dddddd")
  
  const mqttConnect = () => {
    const host = ""
    const mqttOption = {
      clientId: 'emqx_react_' + Math.random().toString(16).substring(2, 8),
      username: "",
      password: "",
      clean: true,
      reconnectPeriod: 1000,
      connectTimeout: 30 * 1000,
    }

    setConnectStatus('Connecting')
    setClient(mqtt.connect(host, mqttOption))
  }

  useEffect(() => {
    if (client) {
      client.on('connect', () => {
        setConnectStatus('Connected')
        console.log('connection successful')

        // Subscribe to topic
        mqttSub({ topic: 'emqx/esp32/3', qos: 0 });
      })

      client.on('error', (err) => {
        console.error('Connection error: ', err)
        client.end()
      })

      client.on('reconnect', () => {
        setConnectStatus('Reconnecting')
      })

      client.on('message', (topic, message) => {
        console.log(`received message: ${message} from topic: ${topic}`)
        handleMessage(message)
      })
    } else {
      mqttConnect()
    }
  }, [client])

  const mqttDisconnect = () => {
    if (client) {
      try {
        client.end(false, () => {
          setConnectStatus('Disconnected')
          setIsSub(false)
          console.log('disconnected successfully')
        })
      } catch (error) {
        console.log('disconnect error:', error)
      }
    }
  }

  // const mqttPublish = (context) => {
  //   if (client) {
  //     // topic, QoS & payload for publishing message
  //     const { topic, qos, payload } = context
  //     client.publish(topic, payload, { qos }, (error) => {
  //       if (error) {
  //         console.log('Publish error: ', error)
  //       }
  //     })
  //   }
  // }

  const mqttSub = (subscription) => {
    if (client) {
      const { topic, qos } = subscription
      client.subscribe(topic, { qos }, (error) => {
        if (error) {
          console.log('Subscribe to topics error', error)
          setIsSub(false)
          return
        }
        console.log(`Subscribe to topics: ${topic}`)
        setIsSub(true)
      })
    }
  }

  const handleMessage = message => {
    const [status, ...nameParts] = `${message}`.split(' ');
    const name = nameParts.join(' ');

    switch (status) {
      case "success":
        setPayload(`Hi ${name}`)
        setBackgroundColor("#3fcc3f")

        new Howl({
          src: [successSound]
        }).play()
        break;
      case "failure":
        setPayload("Error, please try again");
        setBackgroundColor("#de2c26")

        new Howl({
          src: [errorSound]
        }).play()
        break;
      default:
        console.log("Invalid status.")
    }

    setTimeout(() => {
      setBackgroundColor("#dddddd")
      setPayload(null)
    }, 4000);
  }

  return (
    <div className="container" style={{ backgroundColor: backgroundColor }}>
      <div className="info">
        <p>Current Status: {connectStatus}</p>
        <p>Is subscribed: {isSubed ? "yes" : "no"}</p>
        <p>Location: 3</p>
        <button onClick={mqttDisconnect}>DISCONNECT</button>
        <button onClick={mqttConnect}>CONNECT</button>
      </div>
      <div className="title" style={{ color: payload != null ? "white" : "black" }}>
        {payload != null ? payload : "Tap phone or user card"}
      </div>
    </div>
  );
}

export default App;
