// Universal Remote Control - 2022
// Github: @hizbi-github
// Author: Hizbullah Khan
// License: MIT

const { createApp } = Vue;

let newRemoteConfigurationTemplate = 
{
    remoteName: "",
    receiverName: "",
    protocolIR: "",
    receiverAddress: "",
    buttons: 8,
    buttonOneName: "Button 1",
    buttonTwoName: "Button 2",
    buttonThreeName: "Button 3",  
    buttonFourName: "Button 4",  
    buttonFiveName: "Button 5",  
    buttonSixName: "Button 6",  
    buttonSevenName: "Button 7",  
    buttonEightName: "Button 8",
    buttonOneCommand: "",
    buttonTwoCommand: "",
    buttonThreeCommand: "",  
    buttonFourCommand: "",  
    buttonFiveCommand: "",  
    buttonSixCommand: "",  
    buttonSevenCommand: "",  
    buttonEightCommand: ""
}

let fixedViewPortContent = null;
let fixedViewPortHeight = null;
let buttonAndHexCodeFieldsTimer = 0;

const rootApp = createApp({
    data() 
    {
        return {
            webAppURL: window.location.href,
            pageName: "initialPage",
            remoteItemHTMLList: "",
            configuredRemotes: null,
            selectedRemote: null,
            remoteTransmissionRemoteName: "",
            remoteTransmissionReceiverName: "",
            remoteTransmissionProtocol: "",
            remoteTransmissionButtonPressedName: "",
            remoteTransmissionCommandSent: "",
            newRemoteConfigurationObject: newRemoteConfigurationTemplate,
            remoteConfigurationSaveStatusAlert: ""
        }
    },

    mounted() 
    {
            this.mountMethod();
            console.log("Mounted");
    },

    methods: 
    {
        async mountMethod() 
        {    
            this.getAllConfiguredRemotes();
            
            fixedViewPortHeight = window.innerHeight; //console.log(fixedViewPortContent);
            fixedViewPortContent = document.getElementById("viewport").getAttribute("content"); //console.log(fixedViewPortHeight);
            window.addEventListener("resize", this.debounceFunction(this.revertViewPortHeightToFullHeight, 100)); // In case the virtual keyboard on phone browser pushes up the viewport...

            this.revertViewPortHeightToFullHeight(); // Calling it once manually to prevent the one-time flicker on phone browsers.
        },

        revertViewPortHeightToFullHeight()
        {
            let currentViewPort = document.getElementById("viewport");
            currentViewPort.setAttribute("content", fixedViewPortContent + ", height=" + fixedViewPortHeight);
            console.log("Viewport Height Reverted!");
            console.log(currentViewPort.getAttribute("content"));
        },

        goToInitialPage()
        {
            this.pageName = "initialPage";
        },

        goToRemoteSelectionPage()
        {
            this.pageName = "remoteSelectionPage";
        },

        async getAllConfiguredRemotes()
        {
            let response = await (await fetch(this.webAppURL + "api/get-all-configured-remotes")).json();
            this.configuredRemotes = response; 
            console.log("Configured Remotes: ");
            console.log(this.configuredRemotes);

            this.remoteItemHTMLList = "";

            for (remote in this.configuredRemotes)
            {
                this.remoteItemHTMLList = this.remoteItemHTMLList + `<li class="remote-selection-list-item" onclick="remoteTransmissionPageRouter(this.innerHTML)">${this.configuredRemotes[remote].remoteName}</li>`; 
            }
        },

        goToRemoteTransmissionPage(selectedRemote)
        {
            this.pageName = "remoteTransmissionPage";

            for (remote in this.configuredRemotes)
            {
                if (this.configuredRemotes[remote].remoteName == selectedRemote)
                {
                    this.selectedRemote = this.configuredRemotes[remote];
                }
                else
                {
                    this.selectedRemote = this.selectedRemote;  // Keep the previous remote object
                }
            }

            this.remoteTransmissionRemoteName = this.selectedRemote.remoteName;
            this.remoteTransmissionReceiverName = this.selectedRemote.receiverName;
            this.remoteTransmissionProtocol = this.selectedRemote.protocolIR;
            this.remoteTransmissionButtonPressedName = "-";
            this.remoteTransmissionCommandSent = "-"; 

            console.log("Selected Remote: ");
            console.log(this.selectedRemote);
        },

        async transmitIRData(selectedRemoteButtonNameKey, selectedRemoteButtonCommandKey)
        {
            // Clears the Timer if the button is pressed again too soon (before previous interval expires)
            clearTimeout(buttonAndHexCodeFieldsTimer);
            
            //console.log(this.selectedRemote[selectedRemoteButtonNameKey]);
            //console.log(this.selectedRemote[selectedRemoteButtonHexCodeKey]);

            this.remoteTransmissionButtonPressedName = this.selectedRemote[selectedRemoteButtonNameKey];
            this.remoteTransmissionCommandSent = "-";

            let payLoad = 
            {
                protocolIR: this.selectedRemote.protocolIR,
                receiverAddress: this.selectedRemote.receiverAddress,
                receiverCommand: this.selectedRemote[selectedRemoteButtonCommandKey]
            };

            console.log("Payload: ");
            console.log(payLoad);

            try 
            {
                let response = await (await fetch(this.webAppURL + "api/ir-data-from-web-app", 
                {
                    method: "POST", 
                    body: JSON.stringify(payLoad),
                    headers: 
                    {
                        'Content-Type': 'application/json'
                    }
                })).json();

                console.log("Response: ");
                console.log(response);

                this.remoteTransmissionCommandSent = this.selectedRemote[selectedRemoteButtonCommandKey];
            } 
            catch 
            {
                console.warn("Unable to access the server, please check the API URL");

                this.remoteTransmissionCommandSent = "Failed";
            }
            
            // Resets the Button and Hex Code fields after 1 second.
            buttonAndHexCodeFieldsTimer = setTimeout(() =>
            {
                this.remoteTransmissionButtonPressedName = "-";
                this.remoteTransmissionCommandSent = "-";
            }, 1500);

        },

        goToNewRemoteScanningPage()
        {
            this.pageName = "newRemoteScanningPage";
        },

        async receiveIRData(scannedRemoteButtonNameKey, scannedRemoteButtonCommandKey)
        {
            console.log(scannedRemoteButtonNameKey + " " + scannedRemoteButtonCommandKey);

            let response = await (await fetch(this.webAppURL + "api/ir-data-from-mcu")).json();
            console.log(response);

            this.newRemoteConfigurationObject.protocolIR = response.protocolIR;
            this.newRemoteConfigurationObject.receiverAddress = response.receiverAddress;
            this.newRemoteConfigurationObject[scannedRemoteButtonCommandKey] = response.receiverCommand; 

            console.log("IR Hex Code: " + this.newRemoteConfigurationObject[scannedRemoteButtonCommandKey] + " Received for " + this.newRemoteConfigurationObject[scannedRemoteButtonNameKey]);
        },

        goToNewRemoteSavingPage()
        {
            this.pageName = "newRemoteSavingPage";
            this.remoteConfigurationSaveStatusAlert = "";
        },

        async sendNewRemoteConfigurationToServer()
        {
            for (key in this.newRemoteConfigurationObject) 
            {
                if (this.newRemoteConfigurationObject[key] == "" || this.newRemoteConfigurationObject[key] == 0 || this.newRemoteConfigurationObject[key] == null)
                {
                    this.remoteConfigurationSaveStatusAlert = "Empty input fields detected, please fill them all!";
                    console.log("Empty input fields detected! Kindly fill all the fields!");
                    return;
                }
            }

            let payLoad = this.newRemoteConfigurationObject; 

            console.log("Payload: ");
            console.log(payLoad);

            try 
            {
                let response = await (await fetch(this.webAppURL + "api/new-remote-configuration-data-from-web-app", 
                {
                    method: "POST", 
                    body: JSON.stringify(payLoad),
                    headers: 
                    {
                        'Content-Type': 'application/json'
                    }
                })).json();

                console.log("Response: ");
                console.log(response);

                if (response.requestStatus == "Success")
                {
                    this.resetNewRemoteConfigurationObject();
                    this.goToRemoteSelectionPage();
                    this.getAllConfiguredRemotes();
                }
                else
                {
                    this.remoteConfigurationSaveStatusAlert = "The server was unable to save the remote configuration!";
                    console.warn("The server was unable to save the remote configuration!");
                }
            } 
            catch 
            {
                this.remoteConfigurationSaveStatusAlert = "Unable to access the server, please try again!";
                console.warn("Unable to access the server, please check the API URL");
            }
        },

        resetNewRemoteConfigurationObject()
        {
            this.newRemoteConfigurationObject.remoteName = "";
            this.newRemoteConfigurationObject.receiverName = "";
            this.newRemoteConfigurationObject.protocolIR = "";
            this.newRemoteConfigurationObject.receiverAddress = "";
            this.newRemoteConfigurationObject.buttons = 8;
            this.newRemoteConfigurationObject.buttonOneName = "Button 1";
            this.newRemoteConfigurationObject.buttonTwoName = "Button 2";
            this.newRemoteConfigurationObject.buttonThreeName = "Button 3";
            this.newRemoteConfigurationObject.buttonFourName = "Button 4";
            this.newRemoteConfigurationObject.buttonFiveName = "Button 5";
            this.newRemoteConfigurationObject.buttonSixName = "Button 6";  
            this.newRemoteConfigurationObject.buttonSevenName = "Button 7";
            this.newRemoteConfigurationObject.buttonEightName = "Button 8";
            this.newRemoteConfigurationObject.buttonOneCommand = "";
            this.newRemoteConfigurationObject.buttonTwoCommand = "";
            this.newRemoteConfigurationObject.buttonThreeCommand = ""; 
            this.newRemoteConfigurationObject.buttonFourCommand = ""; 
            this.newRemoteConfigurationObject.buttonFiveCommand = ""; 
            this.newRemoteConfigurationObject.buttonSixCommand = "";  
            this.newRemoteConfigurationObject.buttonSevenCommand = "";
            this.newRemoteConfigurationObject.buttonEightCommand = "";
        },

        debounceFunction(functionToBeDebounced, timeInMillis)
        {
            let debounceInterval = timeInMillis || 50; // Interval will be 50ms by default if not specified.
            let debounceTimerID = null;
            return (() =>
            {
                if (debounceTimerID != undefined) // Just to check for the above "null" declaration...
                {
                    clearInterval(debounceTimerID); // Don't execute if the user is still producing inputs/changes. 
                }
                debounceTimerID = setTimeout(functionToBeDebounced, debounceInterval); // Execute the desired function but after the interval.
            });
        }
    }
}).mount("#root-ID")

function remoteTransmissionPageRouter(selectedRemote)
{
    //console.log(selectedRemote);
    rootApp.goToRemoteTransmissionPage(selectedRemote);
}



