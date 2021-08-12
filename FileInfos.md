Here you can find additional information about the files you can select and extract with DFET.

# Extracted files
If you have used it already, you will notice that all audio files are in WAVE(.wav) format, the images are Bitmap(.bmp) and the scripts are text(.txt).
Now first of all, this is not how they are actually defined in the game files. Shocking, I know, but in fact, these files are how I have reinterpreted them.
DFET will convert them in the moment you are going to extract the content and I am glad that I managed to optimize it enough that this whole process takes only a blink of a second.
I can't tell what the original format was, but since the developers loved Apple computers, I can imagine they used typical apple file formats to feed the engine with.

The audio is splitten over several containers, while each container mostly does not exceeds the size of 2^16 bytes.
Each audio container is lossy compressed. Their default format is similar to ADPCM, but it is still different.
The tool will construct the header information, decode each container and combines all audio containers that belong together to one big WAVE-file.
Most of them are in 22050Hz, but there are also a few in 11025Hz. They are all 8bit MONO. I know that the game offers an option to set the audio to 16bit Stereo,
but when I changed this (also in the steam version) I don't hear any noticeable difference. Honestly, I have no idea what this actually changes.

The images or frames are not splitten over multiple containers, so each frame has its own container.
Once again a compressed, custom file format was used and I have never seen something like this before.
I managed to write an algorithm that works for 99% of the frames. Unfortunately a few are just black, or have weird artefacts.
Once the container is decompressed, you have data where every pixel represents one byte, which is very similar to a 8bit Bitmap.
But to construct the bitmap I had to add the color information as well. The color palette can be find in the very first container and is valid for the whole file.
The tool will then flip the image when writing the Bitmap, pulls in the color information and there you have it.

Yes, even the script is constructed in a custom file format and does not look at all like the one you see when extracting.
Also here every script has its own container.
They start with a table that links to DF commands and are given some values. The DF commands are defined in the game main executive.
If the command takes strings instead of integers, those are defined in the bottom of the container.

# .11K
These files are audio archives. They usually hold songs but in a much shorter version. The long, regular versions are in the .TRK files.
You would think that 11K indicates that those are in 11025Hz, but this is not the case.
I can only assume that the engine would pick 11K files over TRK files, if it detects insufficient RAM, which was much, MUCH lesser in 1996 than today.
Game companies always had to look for solutions to make their games also compatibile on slower machines.

# BOOTFILE
This is probably not too exciting for you. There is at least one BOOTFILE in every game that was made with DreamFactory.
It contains at least one script that defines which assets or which content the game has to start with, what it will load and how things work together.

# .MOV
This format is a little bit of an allrounder archive. It can be literally anything. Either it holds just one image of an object you can inspect or
it has a set of images that can be clicked though. It can also have entire animations and it can even hold entire cutscenes with audio and everything.

# .PUP
If you are looking for character information, this is the right place. The game developers called the ingame characters "Puppets".
The game also has dummy puppets that you can not interact with, but the ones you can interact with have their own .PUP file.
In TAOOT each character has even two puppet files, if you see "1" in the filename, it indicates before the sinking, "2" is during the sinking.
This splitting was necessary in order to distribute them over the two CDs the game was shipped out.
These files contain the text that is necessary to talk to them, as well as the scripts that defines the conversation logic.
They also contain the audio that is used for their text and each text element has its own table that defines their facial animation.
DFET can currently not extract their facial frames like open eyes, closed eyes, closed mouth, open mouth etc. etc., but I am sure that they are also there.
Also I do believe that these puppet files contain 3D data with texture and 3D animation for their exterior model.

# .SET
If you know a bit the history of Cyberflix you know that they had that vision of making interactive games, but with a story like being in a movie.
Now the Sets are going with this movie idea. Every Set has multiple Scenes and every Scenes have multiple views.
The Sets are essentially the rooms or sections. The Scenes are the points within the Scene you can transfer to and the views are the individual frames you see when turning around.
As you probably already noticed, the game is not really in 3D (except the puppets, kinda).
All the rooms and the entire environment was pre-rendered in order to allow maximum amount of details.
Back in 1996 it was absolutely impossible to render graphics like this in real-time. This is also the reason why the player is so limited in walking around
and why everything looks so... static, because you are looking at static pictures. If you see motion, like puppets walking around, those are drawn on top of the images.
However: With DFET you can extract all the set frames, also the transition or rotation frames. Usually you don't see them because of the fast movement.
I programmed it in a way that it will sort the scenes into individual subfolders. It will output left turns into the "A" folder, right turns into the "D" folder.
The transition frames to go from one scene to another will be put into the "W" folder. The real structure is different, but I thought that it is much easier like this for most users.

# .SFX
Another audio archive. As you can probably already tell by the name, those hold sound effects. Usually very short audio files come out of this.

# .STG
These files hold scripts and images, like UI elements or images from the mini games. For example the deckplan can be extracted from a STG file as well.

# .TRK
Track files! Those contain the music of the game we love. Some of them also have sound effects and sometimes even voices. 
