# mp4-fps-hack
Hide mp4 videos at specific FPS rate but overclock them at runtime.

## Bypass restriction
This tool is primarily to bypass the TikTok restrictions of 2000 views to unlock the 60fps feature. With this tool you can hide the video as 30 fps, but let it run on 60fps.
It will run only on smartphone player as 60fps. If run in browser, the video will be massivle out of sync and slow.

## Core features
Select a video and a target fps rate, and it will convert the video. If run with minimal parameter it will always use 30 fps settings.

## Important notes
This is a very experimental tool. Possible bugs and sync problem are expected, if used with differnet frame rates and edge case encoder settings.

## Help
converts videos to mp4 and kind of overclocks them, to let them claim have a specific fps, but actually are played on different speed.
tool aims for 30 fps stealth mode, but fps can be adjusted as needed (experimental).
can be backtested, in VLC player it should be slow and stutter in audio, in windows media player, it should be run in the correct fps settings and normal audio.

usage:

mp4_fps_hack.exe <input_file.|mp4|avi|mov> <output_file.mp4> <mode|1|2|3 [1]> <video_channel|0|1|2 [0]> <audio_channel|0|1|2 [1]> <safe_codec|false|true [false]> <target_fps [30]>

arguments:

input_file: the absolute or relative file path to the input video that should be patched (Tested with .mp4, .avi and .mov, can be something else probably)
output_file: the absolute or relative file path to the output video that should saved
mode: integer representing the mode that should be used for the patch (1 = audio stts (recommended), 2 = audio stts and duration, 3 = audio/video stts and duration)
video channel: integer representing the index of the video channel (0 by default)
audio channel: integer representing the index of the audio channel (1 by default)
safe codec: uses codec settings, that are proven to work with this (video quality may suffer)
safe codec: uses codec settings, that are proven to work with this (video quality may suffer)

if the hack dont work for your file, try in probably this order:
- video channel = 1, audio channel = 0
- mode = 2
- mode = 2, video channel = 1, audio channel = 0
- mode = 3
- mode = 3, video channel = 1, audio channel = 0
- as addition: safe codec = true

quick go:
mp4_fps_hack.exe "C:\mypath\input.mp4 C:\mypath\output.mp4"

complex go:
mp4_fps_hack.exe "C:\mypath\input.mp4 C:\mypath\output.mp4" 1 0 1 false 30

if the application isn't work at all, make sure ffmpeg is installed and set in PATH variable, as it is the requirement to bring the video in the correct format
