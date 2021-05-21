import { useRecoilState } from 'recoil';
import { appState } from '../recoil/atoms';
import './Screen.css';
import { BarChart, Droplet, ThermometerHigh } from 'react-bootstrap-icons';
import { bufferToString, copyObject, formatFloat } from '../util';
import { useEffect, useState } from 'react';
import { getClient } from '../paho';

const bathtubVolume = 300;

function Screen() {
    let [app, setApp] = useRecoilState(appState);

    const handlePipePropChange = (pipeName, pipeProp, value) => {
        let newApp = copyObject(app);
        newApp[pipeName][pipeProp] = parseFloat(value);
        setApp(newApp);
    }

    const togglePipe = (pipeName) => {
        let newApp = copyObject(app);
        newApp[pipeName].isOn = !app[pipeName].isOn;
        if(!newApp[pipeName].isOn) {
            newApp[pipeName].debit = 0;
        }
        setApp(newApp);
    }

    useEffect(() => {
        getClient().then(cli => {
            cli.onConnectionLost = onConnectionLost;
            cli.onMessageArrived = onMessageArrived;
            if(!cli.isConnected()) {
                cli.connect({ onSuccess: () => {
                    cli.subscribe("screen");
                }});
            }
        });
    }, []);

    const onMessageArrived = (message) => {
        let payload = bufferToString(message.payloadBytes);
        let splitted = payload.split('/');
        let resultType = splitted[0];
        console.log(resultType);
        switch(resultType) {
            // currentVolume/:volume
            case 'currentVolume':
                setCurrentVolume(parseFloat(splitted[1]));
                break;
            // pipe/:pipeName/on/:debit/:temperature
            // pipe/:pipeName/off
            case 'pipe':
                handlePipeEvent(splitted[1], splitted[2], splitted[3], splitted[4])
                break;
        }
    }

    const handlePipeEvent = (pipeName, isOn, debit, temperature) => {
        let newApp = copyObject(app);
        if(isOn == 'on') {
            newApp[pipeName] = {
                isOn: true,
                debit,
                temperature
            }
        } else {
            newApp[pipeName] = {
                isOn: false,
                debit: 0,
                temperature: app[pipeName].temperature
            }
        }
        setApp(newApp);
    }

    const setCurrentVolume = (volume) => {
        let newApp = copyObject(app);
        newApp.currentVolume = volume;
        setApp(newApp);
    }

    const onConnectionLost = () => {
        alert('Connection lost');
    }

    return (
        <div className="Screen">
            <div className="header">
                <span>Smart Bathtub</span>
            </div>
            <div className="pipes-container">
                <div className="pipe">
                    <div className="pipe-name">Bathtub</div>
                    <div className="pipe-props">
                        <div className="pipe-prop">
                            <input
                                type="range" min="0" max="0.25" value={app.bath.debit || 0} step="0.01"
                                onChange={(event) => handlePipePropChange('bath', 'debit', event.target.value)}/>
                            <div className="pipe-prop-def">
                                <div className="pipe-prop-icon">
                                    <Droplet/>
                                </div>
                                <div className="pipe-prop-text">{formatFloat((app.bath.debit || 0) / 0.25 * 100, 0)}%</div>
                            </div>
                        </div>
                        
                        <div className="toggle-button-container">
                            <div 
                                className={"toggle-button" + (app.bath.isOn ? ' on' : '')}
                                onClick={() => togglePipe('bath')}
                            >
                                {app.bath.isOn ? 'On' : 'Off'}
                            </div>
                        </div>
                        <div className="pipe-prop">
                            <input
                                type="range" min="5" max="50" value={app.bath.temperature || 20} step="1"
                                onChange={(event) => handlePipePropChange('bath', 'temperature', event.target.value)}/>
                            <div className="pipe-prop-def" >
                                <div className="pipe-prop-text"
                                    style={{'position': 'absolute', 'right': '20px'}}
                                >
                                    {app.bath.temperature || 20}°C
                                </div> 
                                <div className="pipe-prop-icon">
                                    <ThermometerHigh/>
                                </div>
                            </div>
                        </div>
                    </div>
                </div>
                <div className="pipe">
                    <div className="pipe-name">Shower</div>
                    <div className="pipe-props">
                        <div className="pipe-prop">
                            <input
                                type="range" min="0" max="0.2" value={app.shower.debit || 0} step="0.01"
                                onChange={(event) => handlePipePropChange('shower', 'debit', event.target.value)}/>
                            <div className="pipe-prop-def">
                                <div className="pipe-prop-icon">
                                    <Droplet/>
                                </div>
                                <div className="pipe-prop-text">{formatFloat((app.shower.debit || 0) / 0.2 * 100, 0)}%</div>
                            </div>
                        </div>
                        
                        <div className="toggle-button-container">
                            <div 
                                className={"toggle-button" + (app.shower.isOn ? ' on' : '')}
                                onClick={() => togglePipe('shower')}
                            >
                                {app.shower.isOn ? 'On' : 'Off'}
                            </div>
                        </div>
                        <div className="pipe-prop">
                            <input
                                type="range" min="5" max="50" value={app.shower.temperature || 20} step="1"
                                onChange={(event) => handlePipePropChange('shower', 'temperature', event.target.value)}/>
                            <div className="pipe-prop-def" >
                                <div className="pipe-prop-text"
                                    style={{'position': 'absolute', 'right': '20px'}}
                                >
                                    {app.shower.temperature || 20}°C
                                </div> 
                                <div className="pipe-prop-icon">
                                    <ThermometerHigh/>
                                </div>
                            </div>
                        </div>
                    </div>
                </div>
            </div>
            <div className="footer">
                <div className="footer-prop">
                    <div className="footer-prop-icon"><BarChart/></div>
                    <div className="footer-prop-value">{ app.currentVolume } / { bathtubVolume }</div>
                </div>
            </div>
        </div>
    )
}

export default Screen;