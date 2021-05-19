import { useRecoilState } from 'recoil';
import { appState } from '../recoil/atoms';
import './Screen.css';
import { Droplet, ThermometerHigh } from 'react-bootstrap-icons';
import { copyObject, formatFloat } from '../util';

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
        setApp(newApp);
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
        </div>
    )
}

export default Screen;