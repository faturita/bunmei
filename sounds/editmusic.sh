ffmpeg -i file.m4a -ss 0:01:22.0 -t 10 -acodec copy -vcodec copy output.m4a
afplay file.m4a

