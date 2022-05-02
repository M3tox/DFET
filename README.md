# About
DFET or "DreamFactory Extraction Tool" allows you to extract data from games that were made with the DreamFactory engine up to version 4.0.
It can extract all file formats that are used by the game "Titanic: Adventure out of time" and its Demo. It partially also supports the game "DUST: A tale of the wired west", which is based on an older version. However, for the final version of DFET it is the goal that DUST will be also fully supported.

You can download the Tool [HERE](https://github.com/M3tox/DFET/releases/tag/0.89).

# Usage
The tool is very straight forward and does not require any programming experience. Once you have started it up it should look like this:
![grafik](https://user-images.githubusercontent.com/75583358/128694060-2675db4e-9553-4ce1-8f4b-4a0ea6324464.png)

In the upper menu you can view the supported formats and the version by clicking on "Info". Under "File" you can extract or close the application. It also supports hotkeys. Press alt, then navigate to File with F and hit X to extract or C to close.

On the left you see the extraction parameters, which should be self explaining. If you just want to extract the audio for example, you can do so by only tic the "Audio" box. If you want to extract everything, keep all tic boxes activated.

Below you can change the default output location. This is the location where the extracted files will be written to. If you don't use this, it will extract the files to the same location where the files to extract are. If you extract the files directly from the CD, you must use this option, because it can obviously not write onto a CD and there is currently no error message indicating that.
Use the button "Change Path" to select the folder where you want the files to extract to, unless you keep the default location.

When everything is set, use the button to the right "Extract File". Select the file you want to extract from and the tool will directly start to work. I tried to optimize it a lot, so that it should only take a blink of a second.
After the extraction you should see an extraction report, like this:
![grafik](https://user-images.githubusercontent.com/75583358/128695709-f54d78d4-384c-45ea-9fa8-8075be88cedb.png)

It will write the extracted files into a folder, that is named after the file you have selected, but with a _ before. E.g.: "CONTROL.SET" -> "_CONTROL".

# Supported formats and platforms
This tool was tested for Windows 10, but in theory, it should also work down to Windows 7. I was informed that it also works on the new Windows 11. There is currently no MAC or Linux version available.
It's a 32bit application, which also works on 64bit operating systems.

Files you can currently extract:

- 11K
- BOOTFILE
- CST
- MOV (Titanic: Adventure out of time only)
- PUP
- SET (Titanic: Adventure out of time only)
- SFX
- SHP
- SND
- STG
- TRK

You can also view this list with the tool, by clicking on "Info" in the upper menu bar.
If you want more detailed information about this you can click [HERE](https://github.com/M3tox/DFET/blob/main/FileInfos.md).

# Extraction
Now, what can you actually get out of these files? Depending to your selection in the Extraction parameters (the tic boxes), you can extract:
- Scripts
- Audio
- Frames

Keep in mind that there may be still data that currently can't be detected/extracted.

# Code
Feel free to use the code for your programming projects under the GPL-3.0 License. The backend with all its DF file processing functions is a library, while DFET.cpp is just taking care of the DFET window logic. The reason for this design decision is because I have multiple projects that utilize these files for all kind of things, for example a camera controller that can capture new frames based on the SETs coordinates, or an editor called "DFedit". Since they all use the same library I can apply changes deep in the backend in every project at the same time.
This is why you may notice a few weird things, for example why the decompressed images will be held in RAM at first and then need a secondary function to actually write it to disk. In other projects I just want to display the images instead of writing them.

# Thank you
I want to thank the community to keep motivating me to push this project forward.

I thank MRXstudios for publishing his work. [This blog](https://mrxstudios.home.blog/2021/03/05/reverse-engineering-dust-uncovering-game-scripts/) inspired me to start working on DFET and I may have never started it, without reading this.

I also want to thank Serena Barett and Jasper Carmack from the THG Discord server for testing the first version of the tool (v0.70 9th Aug 2021) on their machines and their feedback.
