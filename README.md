# Amiibo Scratch Extension

This is a custom Scratch/TurboWarp extension that lets you use Amiibos directly inside your projects.  
The extension is written in C++ and can run on **Nintendo 3DS** (working) and **Nintendo Wii U** (experimental).  

---

## How to use it in TurboWarp

1. Copy the file from this folder:  
   `Scratch-Extensions/Amiibos`  

2. Open **TurboWarp** (or another Scratch mod that supports custom extensions).  

3. Go to the **Custom Extensions** menu.  

4. In the new window choose the **Text** tab at the top.  

5. Paste the code into the text field.  

6. You do **not** need to select "Run without sandbox". Just confirm.  

Now the Amiibo blocks will show up in the block palette.  

---

## Supported platforms

- **3DS:** Works with the system NFC hardware.  
- **Wii U:** Should work, but is not fully tested. Internally the code runs a background thread that calls the NFC update function so that tag detection is always active while scanning.  

---

## Available blocks

- **`Amiibo supported?`**  
  Returns `true` if the current system has NFC available.  

- **`Start Amiibo Scan`**  
  Initializes the NFC system and begins scanning for Amiibo.  

- **`Stop Amiibo Scan`**  
  Stops the NFC system and cleans up resources.  

- **`Get Amiibo UID`**  
  Returns the unique ID of the Amiibo if one was detected. If none is detected, it returns an empty string.  

---

## How to use the blocks

You can use them to build menus or gameplay features. For example:  

- When the player presses a button, start scanning.  
- Show a loading animation.  
- Repeatedly check `Get Amiibo UID` until it returns something.  
- Save the UID to a variable and stop scanning.  

This lets you react to real Amiibos inside your Scratch project.  

---

## Internal behavior

- On **3DS** the NFC system runs in the background after you call *Start Scan*. Each call to *Get UID* only checks the current NFC state.  
- On **Wii U** there is a small background thread that continuously calls the system NFC update function so that Scratch can get the current tag without missing events.  

---

## Notes

- Works on **3DS**.  
- **Wii U** support is not guaranteed, because the NFC library may behave differently.  
- No sandbox bypass or special permissions are required in TurboWarp.

*Extension code for Amiibo functionality developed by Br0tcraft.  
Forked and based on the original repository by Grady Link.*