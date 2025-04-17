This is a simple text adventure game similar to Zork in nature.
I work on a raspberry pi 4.
I heavily used ChatGPT to help come up with names and descriptions for this game and to write all of the networking code for the LLM api calls.

this game depends on curl and cJSON, to install, the following worked for me
 - sudo apt-get install libcurl4-openssl-dev
 - sudo apt-get install libcjson-dev

When you play, there is a command called jovi, that is meant to be hooked up to a local LLM Model,
running with Ollama, I used llama3 which is nice and small. I actually cant run it on my pi,
pi's do not do that well, I have another computer locally on my network that I run the LLM on,
and sorry for this but the networking stuff is hardcoded into the adventure.c file.

To set this up locally, install Ollama, use Ollama to install a LLM (I recommend llama3 as it is small but powerful), 
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

the output of the help command looks like this:

  ```
  Enter a command: help
  *You entered: help
  Commands:
  - help   -> display this help menu
  - look   -> describe the current card and reveal items
  - move   -> change cards
  - map    -> displays cards with lit fireplaces, that you can jump to
  - status -> print current status, health, money, mana, etc...
  - sword  -> use your sword if you have it, s for short
  - cast   -> cast a spell, not helpful during fights
  - buy    -> when in a store, you can buy items
  - jovi   -> your best friend, she's a talking dog
  - quit   -> quit the program
  ```

 look is your bread and butter, use it to find items and read descriptions of the locations of where you currently are.
 
 move lets you change cards. use it to move around.
```
 Enter a command: move
*You entered: move
Places to move to: 
 - front - Your Frontdoor
 - back - Your Backyard
```
 type the text on the left like "front" or "back", the text on the right is the title of the card to move to.
 
 sword (s for short) is how you attack, fight enemies this way.
 
 cast spells with cast, it will list the options for you. you need the ring to use it.
  - fire is used to light fireplaces. Look for a fireplace mentioned in the description from the look command
    - if the current card has a fireplace, cast fire to light
    - then you can use the map to jump back to that location quickly
  - grow is used in gardens to food which gives you a lot of health
  - open is used to open chests and secret passages
 
 when in a store, you can buy items, right now that is a very simple system, but you can do it.
 
 jovi command is for talking to Jovi, Jovi is your companion, a talking dog.
 
 CTRL-D to exit quickly, or just use exit
 
 CTRL-C to enable cheats (look in the code for what you can do, feel free to add to them, and any of this can be copied and modified, It is fine with me)

There are also events that happen based on the loop timer (increments every time you enter a command or press enter)
Hit enter frequently to make sure these trigger when you first get to the card.

There is a ring, a map, and a sword. To play effectively you will need all three. The ring is delivered to you in your home (first card of the game) by the blue wizard after 10 game loops. The sword is in your home as well and all you have to do is look. The map is in a tree down the road (left out the door). make sure you use look after you climb the tree and be sure to cast fire whenever a fireplace is available (typically at homsteads).

For the whispering marsh section, I provided a map (WhisperingMarsh_005.png), the map sucks. 
It sucks hard. But use it as this area is easy to get lost in.
The WhisperingMarsh_005.png file was actually generated by ChatGPT. I say it sucks but honestly I gave ChatGPT snippets of the code
and after a long prompting battle it was able to generate the png file which I was quite impressed with.

Deliver books to Alister in the south. He will pay you well.

In the future I do plan to finish the game, and break it into seperate files, and provide a config file for easy networking setup.
I think it would also be cool to extend the LLM stuff, maybe have many NPC's that you can talk with instead of just Jovi the talking dog.

also, sorry for one big c file, I dont know what I am doing
