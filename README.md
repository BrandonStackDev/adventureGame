This is a simple text adventure game similar to Zork in nature.
I work on a raspberry pi 4.
I heavily used ChatGPT to help come up with names and descriptions for this game.

this game depends on curl and cJSON, to install, the following worked for me
 - sudo apt-get install libcurl4-openssl-dev
 - sudo apt-get install libcjson-dev

When you play, there is a command called jovi, that is meant to be hooked up to a local LLM Model,
running with Ollama, I used llama3 which is nice and small. I actually cant run it on my pi,
pi's do not do that well, I have another computer locally on my network that I run the LLM on,
and sorry for this but the networking stuff is hardcoded into the adventure.c file.

to set this up locally, install Ollama, use Ollama to install a LLM (I recommend llama3 as it is small but powerful), 
run on a good computer prefferably with a nicer GPU that can handle llama3,
and change the following things in the adventure.c file
 - go to the handleJovi function all the way at the bottom of the file
 - change the url http://127.0.0.1:11434/api/generate
   - curl_easy_setopt(curl, CURLOPT_URL, "http://127.0.0.1:11434/api/generate");
   - find the ip of the computer running the Ollama instance, and replace the ip address in the url with that
   - update the port also? I think Ollama always uses 11434 but look out for that
   - look out for other things like firewalls if you have trouble
 - if you are running a different LLM
   - find the line that specifies llama3 as the model, and replace "llama3" with whatever model you are running
   - cJSON_AddStringToObject(json_data, "model", "llama3");

Additionally, you may need to bind to all interfaces with 0.0.0.0.
In a CMD prompt or bash shell
 - SET OLLAMA_HOST=0.0.0.0 //windows
 - export OLLAMA_HOST=0.0.0.0 //linux

And to run manually with that param you might need to stop the instance and start from the cli
 - ollama stop llama3 //replace llama3 with your model if different
 - ollama serve

To play the game, use the help command for a list of all commands.
You will need to get the ring, the sword, and the map.
Currently the only objective is to gather books and explore, but there is no way to win,
and the game is not complete.

For the whispering marsh section, I provided a map, the map sucks. It sucks hard. But use it as this area is easy to get lost in.

In the future I do plan to finish the game, and break it into seperate files, and provide a config file for easy networking setup.
I think it would also be cool to extend the LLM stuff, maybe have many NPC's that you can talk with instead of just Jovi the talking dog.

also, sorry for one big c file, I dont know what I am doing
