This is a simple text adventure game similar to Zork in nature.
I work ona raspberry pi 4.
I heavily used ChatGPT to help come up with names and descriptions for this game.

this game depnds on curl and cJSON, to install, the following worked for me
 - sudo apt-get install libcurl4-openssl-dev
 - sudo apt-get install libcjson-dev

When you play, there is a command called jovi, that is meant to be hooked up to a local LLM Model,
running with Ollama, I used llama3 which is nice and small. I actually cant run it on my pi,
pi's do not do that well, I have another computer locally on my network that I run the LLM on,
and sorry for this but the networking stuff is hardcoded into the adventure.c file.

to set this up locally, run on a good computer prefferably with a nicer GPU that can handle llama3,
and change the following things in the adventure.c file
 - go to the handleJovi function all the way at the bottom of the file
 - change the url http://10.0.0.253:11434/api/generate
   - find the ip of the computer running the Ollama instance, and replace the ip address in the url with that
   - update the port also?
 - if you are running a different LLM
   - find the line that specifies llama3 as the model, and replace "llama3" with whatever model you are running

also, sorry for one big c file, I dont know what I am doing
