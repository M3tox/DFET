Here you can find additional information about the files you can select and extract with DFET.

# Extracted files
If you have used it already, you will notice that all audio files are in WAVE(.wav) format, the images are Bitmap(.bmp) or PNG and the scripts are text(.txt).
Now first of all, this is not how they are actually defined in the game files. Shocking, I know, but in fact, these files are how I have reinterpreted them.
DFET will convert them in the moment you are going to extract the content.
I can't tell what the original format was *before* it went into the engine, but since the developers loved Apple computers, I can imagine they used typical apple file formats to feed the engine with.

The audio is splitten over several containers, while each container mostly does not exceeds the size of 2^16 bytes.
Each audio container is lossy compressed. Their default format appears to be ADPCM, but with custom tables.
The tool will construct the header information, decode each container and combines all audio containers that belong together to one big WAVE-file.
Most of them are in 22050Hz, but there are also a few in 11025Hz. They are all 8bit MONO. I know that TAOOT offers an option to set the audio to 16bit Stereo, but when I changed this (also in the steam version) I don't hear any noticeable difference. Honestly, I have no idea what this actually changes.

The images or frames are not splitten over multiple containers, so each frame has its own container.
Two different decompression algorithms are necessary to convert the images to a readable format. Set and Movie frames are compressed differently than the other file formats. Once the container is decompressed, you have data where every pixel represents one byte, which is very similar to a 8bit Bitmap.
To construct the full bitmap just the color information needs to be added. The color palette can be find in the very first container and is valid for the whole file. Images that can contain transparency will be converted to PNG.

Scripts are also constructed in a custom file format and does not look at all like the one you see when extracting. In their binary format they are even bigger, so this is not about compression, it is about reading efficiency. The scripts were interpreted at this point already.
Also here every script has its own container.
They start with a table that links to DF commands and are given some values. The DF commands are defined in the game main executive.
If the command takes strings instead of integers, those are defined in the bottom of the container.

# .11K
These files are audio archives. They usually hold songs but in a much shorter version. The long, regular versions are in the .TRK files.
You would think that 11K indicates that those are in 11025Hz, but this is not the case.
The BOOTFILE defines if the 11K or TRK files should be used based on the systems memory. If the game detects less than 6000 KB of RAM, it will choose 11K. If you are younger you might think this number is a typo. 6000 KB? Thats not even 6 MB of RAM! Yep, but this game is from 1996 where RAM was MUCH lesser than today.
Game companies always had to look for solutions to make their games also compatibile on slower machines.

# BOOTFILE
This is probably not too exciting for you. There is at least one BOOTFILE in every game that was made with DreamFactory.
It contains at least one script that defines which assets or which content the game has to start with, what it will load when and how things work together.

# .MOV
Movie files. This format is a little bit of an allrounder. It can be literally anything. Either it holds just one image of an object you can inspect or
it has a set of images that can be clicked though. It can also have entire animations and it can even hold entire cutscenes with audio and everything.

# .PUP
If you are looking for character information, this is the right place. The game developers called the ingame characters "Puppets".
The game also has dummy puppets that you can not interact with, but the ones you can interact with have their own .PUP file.
In TAOOT each character has even two puppet files, if you see "1" in the filename, it indicates before the sinking, "2" is during the sinking.
This splitting was necessary in order to distribute them over the two CDs the game was shipped out.
These files contain the text that is necessary to talk to them, as well as the scripts that defines the conversation logic.
They also contain the audio that is used for their text and each text element has its own table that defines their facial animation.
The Frames for facial animations are also there, surprisingly a lot of unused ones.
Their exterior model however is not included. Those are in the cast files, which includes sets of different sprites, that scales up and down depending how close it is to you. The distance as well as blocking view elements is calculated with an additional Z frame in the .SET file. Seee below for more infos.

# .SET
If you know a bit the history of Cyberflix you know that they had that vision of making interactive games, but with a story like being in a movie.
Now the Sets are going with this movie idea. Every Set has multiple Scenes and every Scenes have multiple views.
The Sets are essentially the rooms or sections. The Scenes are the points within the Set you can transfer to and the views are the individual frames you see when turning around.
As you probably already noticed, the game is not really in 3D.
All the rooms and the entire environment was pre-rendered in order to allow maximum amount of details.
Back in 1996 it was absolutely impossible to render graphics like this in real-time. This is also the reason why the player is so limited in walking around, like on rails and why everything looks so... static, because you are looking at static pictures. If you see motion, like puppets walking around, those are drawn on top of the images.
While in earlier versions like in DUST the scenes are defined by a matrix, in TAOOT every camera frame has its own position/rotation information. At this point, it is not fixed to a grid anymore, the scenes can be anywhere. Diagonal and curve transitions are also possible.
Here you can see the layout of the TAOOT lounge with its camera positions. The blue lines are the path the player can go. The numbers in the boxes are the scene IDs. Between the scenes you see another number, which is the amount of views.

![CameraMovementLounge](https://user-images.githubusercontent.com/75583358/148464272-1e797730-9504-41e6-ae67-53b72c770bc5.png)

However: With DFET you can extract all the set frames, also the transition or rotation frames. Usually you don't see them because of the fast movement. Z frames will be also extracted.
Now a Z frame is very similar to a Z buffer. It contains information about the image depth for every pixel. This would also determine if a puppet would be displayed at this particular location or not. Imagine a chair standing in front of a puppet, the chair has a lower distance value than the puppet, because the chair is closer to you, so the puppets pixel wont be drawn.
I have visualized the depth map to explain it better, just imagine darker colors are closer and I am sure it will make fully sense to you. Below that you see how the corresponding colored image looks like.

![262_depthMap](https://user-images.githubusercontent.com/75583358/148464307-0435d674-855d-4b90-81f5-779df2927d3c.png)

# .SFX
Another audio archive. As you can probably already tell by the name, those hold sound effects. Usually very short audio files come out of this.

# .STG
Those are Stage files, these hold scripts and images, like UI elements or images from the mini games. For example the deckplan can be extracted from a STG file as well.

# .SHP
People who played "Titanic: Adventure out of time" might assume that those are "ship files". Well, this is wrong. The developers call these shop files. Usually they contain props. Images that will be drawn on top of the background images taken from the SET files. In many cases its things you can interact with and that will suddenly appear on screen, like itmes to pick up, buttons to press or doors.

# .SND
Sound files. This is an older format that is much less present in newer DF versions. It can hold all kinds of sounds, but it it was rather limited to what it can do. I guess that is the reason why this was changed later to multiple different sound formats for different purpose.

# .CST
Cast files. They hold additional data for the puppets like their exterior sprites and animations (walking, idle, etc.).

# .TRK
Track files! Those contain the music of the game we love. Some of them also have sound effects and sometimes even voices. 
