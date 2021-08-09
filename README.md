# About
DFET or "DreamFactory Extraction Tool" allows you to extract data from games that were made with the DreamFactory engine up to version 4.0.
However, the development was mainly focused on the game "Titanic: Adventure out of time", which was made with DreamFactory version 4.0, but it also supports the game "DUST: A tale of the wired west", which is based on an older version.

You can download the Tool HERE.

# Usage
The tool is very straight forward and does not require any programming experience. Once you have started it up it should look like this:
![grafik](https://user-images.githubusercontent.com/75583358/128694060-2675db4e-9553-4ce1-8f4b-4a0ea6324464.png)

In the upper menue you can view the supported formats and the version by clicking on "Info". Under "File" you can extract or close the application. It also supports hotkeys. Press alt, then navigate to File with F and hit X to extract or C to close.

On the left you see the extraction parameters, which should be self explaining. If you just want to extract the audio for example, you can do so by only tic the "Audio" box. If you want to extract everything, keep all tic boxes activated.

Below you can change the default output location. This is the location where the extracted files will be written to. If you don't use this, it will extract the files to the same location where the files to extract are. If you extract the files directly from the CD, you must use this option, because it can obviously not write onto a CD and there is currently no error message indicating that.
Use the button "Change Path" to select the folder where you want the files to extract to, unless you keep the default location.

When everything is set, use the button to the right "Extract File". Select the file you want to extract from and the tool will directly start to work. I tried to optimize it a lot, so that it should only take a blink of a second.
After the extraction you should see an extraction report, like this:
![grafik](https://user-images.githubusercontent.com/75583358/128695709-f54d78d4-384c-45ea-9fa8-8075be88cedb.png)

It will write the extracted files into a folder, that is named after the file you have selected, but with a _ before. E.g.: "CONTROL.SET" -> "_CONTROL".

# Supported formats and platforms
This tool was tested for Windows 10, but in theory, it should also work down to Windows 7. There is currently no MAC or Linux version available.

In it's current version it is mainly focused on the game Titanic: Adventure out of time, which was made with DreamFactory version 4.0.
You can also view the compatibility with the tool by clicking on "Info" in the upper menue bar.

Files you can currently extract:
11K
BOOTFILE
MOV (Titanic: Adventure out of time only)
PUP
SET (Titanic: Adventure out of time only)
SFX
STG
TRK

# Extraction
Now, what can you actually get out of these files? Depending to your selection in the Extraction parameters (the tic boxes), you can extract:
Scripts
Audio
Frames

Keep in mind that this tool is not 100% perfect. There is still data that currently can't be extracted.
For example puppet frames are not supported in this current version.
Also it seems like the puppet files have 3D data. Those are not going to be extracted any time soon, atleast not by this tool.

# Development mode
For those who want to get more details about the files or want to do further operations, you can start the devopment mode by using "-dev" as start parameter. The development mode has no user interface and is completely text based, which means all operations are via the default console. However, if you just want to extract the files, you won't need this. All it does is giving an additional tool set for developers.

# Thank you
I want to thank the community to keep motivating me to push this project forward.
Although he never responded to my messages, I want to thank MRXstudios to publish his work. [This article](https://mrxstudios.home.blog/2021/03/05/reverse-engineering-dust-uncovering-game-scripts/) inspired me to start working on DFET and I may have never started it, without reading this.
I also want to thank Serena Barett and Jasper Carmack from the THG Discord server for testing the tool on their machines and their feedback.
