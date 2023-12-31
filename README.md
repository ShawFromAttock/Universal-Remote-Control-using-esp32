# Universal-Remote-Control-using-esp32
## Using esp32 to create an all-in-one remote control
Remote controls have become an integral part of our daily lives, as they allow us to easily control various electronic devices such as TVs, air conditioners, and DVD players. These devices typically use infrared (IR) signals to communicate with the remote control. However, each device usually comes with its own remote control, which can be inconvenient and clutter the living space.
To address this issue, we propose a universal remote control using ESP32 microcontroller, which can control various IR enabled devices using a single remote. The ESP32 microcontroller is connected to an IR LED, which sends the IR signals to the target device. The remote control can be easily programmed to control different devices by simply inputting the IR codes for those devices. The remote control also has a user-friendly interface, allowing users to easily select and control the desired devices.

## Working:
On our webapp, we have two options, either to use a saved remote, or add a new one.
If we select using a saved remote option, it has already loaded all the saved remotes when we open the webapp using /api/get-all-configured-remoted. We can select our remote from the list and it loads all the buttons and the hex codes associated with each button.
If we want to add a new remote, the webapp provides us with a new page in which we can save hex codes associated to any button. We can edit out button name. We have a scan button which saves the previous received IR Code on the Receiver Module and saves it to the corresponding Button and Hex field.
We can also select which type of protocol the remote uses.
After saving the hex codes, we are asked to save the remote configuration by naming it, naming the receiver/device on which it is used. The address space is autofilled.
Then we can go to our scanned remoted, refresh the list and find our remote.
Each button will be sending the IR code assigned to it.

