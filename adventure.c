#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <stdbool.h>

//sudo apt-get install libcurl4-openssl-dev
#include <curl/curl.h>
//sudo apt-get install libcjson-dev
#include <cjson/cJSON.h>

//networking constants
const int MAX_NET_RESP_LENGTH = 131072; //always powers of 2 if we can help it...

//global booleans
bool gameLooper = true;
bool cheating = false;
// Handler for SIGINT, caused by 
// Ctrl-C at keyboard 
void handle_sigint(int sig) 
{
    if(!cheating)
    {
        printf("\n***Cheats enabled***\n");
        cheating = true;
    }
}


//The below callback works for what I need, if Jovi acts up lots of debugging to uncomment
size_t write_callback(void *ptr, size_t size, size_t nmemb, char *data) {
    // Calculate the total size of the incoming data chunk (in bytes)
    size_t total_size = size * nmemb;
    
    // Debug: Print the size of the data being received
    //printf("Debug: Incoming data size = %zu bytes\n", total_size);
    
    // If the data pointer or ptr is NULL, we can't proceed with appending data
    if (data == NULL || ptr == NULL) {
        // Debug: Print a message if either data or ptr is NULL
        //printf("Debug: Error - data or ptr is NULL, returning 0.\n");
        printf("Ruff Ruff!\n");
        return 0;  // Return 0 to indicate that no data was processed
    }

    if(strlen(data)+total_size >= MAX_NET_RESP_LENGTH) 
    { 
        printf("(scratches behind ears)\n");
        //printf("Debug: Error - too much data received\n");
        return total_size; //return 0 here results in no information being printed to the screen, but Im not sure if this is correct, 0 felt more correct?
    }

    // Debug: Print the first few characters of the incoming data (to verify it's correct)
    //printf("Debug: Incoming data chunk (first 100 chars): %.100s\n", (char*)ptr);

    // Use strncat to append the incoming data to the existing 'data' buffer
    // strncat will safely append 'total_size' bytes from ptr to data
    strncat(data, (char*)ptr, total_size);

    // Debug: Print the length of the data after appending
    //printf("Debug: Data after appending (length = %zu): %.100s\n", strlen(data), data);

    // Return the total size of data processed
    // This tells libcurl how much data we've handled
    return total_size;
}

//const for maxes and such
const int MAX_INPUT_UNIQUE = 512; //512
const int MAX_INPUT_UNIQUE_JOVI = 4096; //4096

//typedefs and enums
typedef struct {
    int cardType;
    char name[32];
} Transition;

typedef struct {
    int enemyType;
    int health;
    int damage;
} Enemy;

typedef struct {
    int cardType;
    bool isStore;
    char name[64];
    char description[512];
    int numLinkedCards;
    Transition linkedCards[16];
    int numLinkedItems;
    int linkedItems[16];
    int numLinkedEnemies;
    Enemy linkedEnemies[16];
} Card;

Card *currentCard;
Card deck[128];
int TotalNumberOfCards = 102;//increment deck size by powers of two, when this value is close to the max

enum CardTypes {
    CARD_HOME = 0,
    CARD_FRONTDOOR = 1,
    CARD_FIRST_ROAD_LEFT = 2,
    CARD_FIRST_ROAD_RIGHT = 3,
    CARD_FIRST_CLIMB_TREE = 4,
    CARD_FIRST_TROUBLE = 5,
    CARD_DEAD_END = 6,
    CARD_FIRST_PATH = 7,
    CARD_ROAD_AFTER_TROUBLE = 8,
    CARD_OLD_MAN_JEFFERS = 9,
    CARD_JEFFERS_GARDEN = 10,
    CARD_TOWN_SQUARE = 11,
    CARD_FIRST_STORE = 12,
    CARD_MIRKWOOD_ENTER = 13,
    CARD_CAVE_ENTER = 14,
    CARD_CAVE_MIDDLE = 15,
    CARD_CAVE_DEEP = 16,
    CARD_MIRKWOOD_1 = 17,
    CARD_MIRKWOOD_2 = 18,
    CARD_MIRKWOOD_3 = 19,
    CARD_MIRKWOOD_EXIT = 20,
    CARD_TOWN_HOUSE = 21,
    CARD_BACKDOOR = 22,
    CARD_SECOND_PATH = 23,
    CARD_WILD_GARDEN = 24,
    CARD_BRAMBLETHORN_OVERLOOK = 25,
    CARD_IRONWOOD_CLEARING = 26,
    CARD_ECHOING_HOLLOW = 27,
    CARD_CINDERSPIRE = 28,
    CARD_FIRST_SHED = 29,
    CARD_SECOND_SHED = 30,
    CARD_OTHER_SIDE_RIVER = 31,
    CARD_OLD_MAN_MCCANN = 32,
    CARD_WRENWOOD = 33,
    CARD_FIRST_TRAIL = 34,
    CARD_SECOND_STORE = 35,
    CARD_MCCANN_GARDEN = 36,
    CARD_THIRD_SHED = 37,
    CARD_LOST_WOODS1 = 38,
    CARD_LOST_WOODS2 = 39,
    CARD_LOST_WOODS3 = 40,
    CARD_FIRST_BRIDGE = 41,
    CARD_WILLIAMS = 42,
    CARD_WILLIAMS_STORE = 43,
    CARD_WILLIAMS_TOWN_HALL = 44,
    CARD_WILLIAMS_ROAD = 45,
    CARD_STACK_HOMESTEAD = 46,
    CARD_ELDERFERN_FOREST_ENTER = 47,
    CARD_EMBERFELL_CASTLE_ENTER = 48,
    CARD_ELDERFERN_1 = 49,
    CARD_ELDERFERN_2 = 50,
    CARD_ELDERFERN_3 = 51,
    CARD_ELDERFERN_4 = 52,
    CARD_EMBERFELL_HALL = 53,//The Emberhall
    CARD_EMBERFELL_CHAMBER = 54,//The Chamber of Ashes
    CARD_EMBERFELL_LIBRARY = 55,//The Library of Forgotten Lore
    CARD_EMBERFELL_GARDEN = 56,//The Forgotten Garden
    CARD_EMBERFELL_STAIRS = 57,//Spiral Staircase
    CARD_EMBERFELL_DUNGEON = 58,//The Dungeon of Emberfell
    CARD_EMBERFELL_TOWER = 59,//The Ashen Tower
    CARD_EMBERFELL_ROAD = 60,//Road outside of the dungeons exit
    CARD_GREY = 61, //Alistair Grey, this is where you take books to get paid
    CARD_STACK_RIVER = 62,
    CARD_OBSERVATORY = 63, //The Celestial Observatory
    CARD_CAVERNS = 64, //Crystal Caverns
    CARD_TOMBS = 65, //Ancient Tombs
    CARD_VILLAGE = 66, //The Enchanted Village
    CARD_PEAKS = 67, //Skyward Peaks
    CARD_GLYPHS = 68, //Ancient Glyphs
    CARD_FORSAKEN_ROAD = 69, //Forsaken Road
    CARD_HIDDEN_PASSAGE = 70, //Hidden Passage
    CARD_VILLAGE_STORE = 71, //store in the village
    CARD_FORSAKEN_ROAD_2 = 72, //Forsaken Road
    CARD_MOUNTAIN = 73, //Stoneheart Mountain
    CARD_MARSH_ENTER = 74, //The Whispering Marsh Entrance
    CARD_MARSH_1 = 75, //The Whispering Marsh
    CARD_MARSH_2 = 76, //The Whispering Marsh
    CARD_MARSH_3 = 77, //The Whispering Marsh
    CARD_MARSH_4 = 78, //The Whispering Marsh
    CARD_MARSH_5 = 79, //The Whispering Marsh
    CARD_MARSH_6 = 80, //The Whispering Marsh
    CARD_MARSH_7 = 81, //The Whispering Marsh
    CARD_MARSH_8 = 82, //The Whispering Marsh
    CARD_MARSH_9 = 83, //The Whispering Marsh
    CARD_MARSH_10 = 84, //The Whispering Marsh
    CARD_MARSH_11 = 85, //The Whispering Marsh
    CARD_MARSH_12 = 86, //The Whispering Marsh
    CARD_MARSH_13 = 87, //The Whispering Marsh
    CARD_MARSH_EXIT = 88, //The Whispering Marsh Exit
    CARD_HEARTHSTONE = 89, //Town of Hearthstone
    CARD_HEARTHSTONE_STORE = 90, //store in Hearthstone
    CARD_COTTAGE_1 = 91, //Thistlehill Cottage
    CARD_COTTAGE_2 = 92, //Sunbeam Cottage
    CARD_FARM_BAKER = 93,//Family Farm
    CARD_AJ = 94, //Old Man Aj's
    CARD_HEARTHSTONE_ROAD = 95, //Road east of Town of Hearthstone
    CARD_FARM_TURNEY = 96, //Family Farm
    CARD_PUB = 97, //Pub near Hearthstone
    CARD_WRENWOOD_ROAD = 98, //Road east of Wrenwood
    CARD_BELLEVILLE= 99, //Belleville
    CARD_BELLEVILLE_STORE = 100, //Belleville
    CARD_BELLEVILLE_LIBRARY = 101, //Belleville
};

enum ItemTypes {
    ITEM_SWORD = 0,
    ITEM_MAP = 1,
    ITEM_HEALTH_POTION = 2,
    ITEM_MANA_POTION = 3,
    ITEM_COINS = 4,
    ITEM_BOOK = 5,
};

enum EnemyTypes {
    ENEMY_OGRE = 0,
    ENEMY_WOLF = 1,
    ENEMY_BEAR= 2,
    ENEMY_GOBLIN = 3,
    ENEMY_ORC = 4,
};

//status info
int health = 100;
int mana = 100;
int money = 400;
int xp = 0;
int books = 0;
int booksDeliveredToAlistair = 0;

//global state bools
bool hasSword = false;
bool hasMap = false;
bool hasRing = false;

//global state bools for events
bool ringEventHasHappened = false;
bool kingEventHasHappened = false;
bool tomBomEventHasHappened = false;
bool squirrelEventHasHappened = false;
bool echoEventHasHappened = false;
bool treeFallEventHasHappened = false;
bool elderfernEventHasHappened = false;
bool stackTreeFallEventHasHappened = false;
bool chasmEventHasHappened = false;
bool jovi1EventHasHappened = false;
bool jovi2EventHasHappened = false;
bool jovi3EventHasHappened = false;
bool jovi4EventHasHappened = false;
bool jovi5EventHasHappened = false;
bool jovi6EventHasHappened = false;
bool pub1EventHasHappened = false;
bool pub2EventHasHappened = false;
bool ghostEventHasHappened = false;
//global state bools for fireplaces
bool homeFirePlaceIsLit = false;
bool jeffersFirePlaceIsLit = false;
bool cinderSpireFirePlaceIsLit = false;
bool mccannsFirePlaceIsLit = false;
bool williamsFirePlaceIsLit = false;
bool stackFirePlaceIsLit = false;
bool villageFirePlaceIsLit = false;
bool ajFirePlaceIsLit = false;
bool bellevilleFirePlaceIsLit = false;

//global state bools for treasure chests
bool emberfellTowerChestOpened = false;

//how many times has the game loop ran
int loops = 0;

//proto functions
void initDeck();
void formatInput(char* input);
void printAndHandleItem(int itemType);
char* getEnemyName(int enemyType);
char* getEnemyAttack(int enemyType);
void handleFightSequence();
int getDamage();
void handleEvents();
void handleCast(char* spell);
void removeNonPrintableChars(char *str);
void process_json_stream(char *response);
void handleJovi(char* input);

//here we go!
int main()
{
    //handle signal
    signal(SIGINT, handle_sigint); 
    //this guy holds the input
    char buffer[MAX_INPUT_UNIQUE];
    //welcome the player
    printf("Welcome to Adventure!\n\n\n");
    initDeck();

    //enter game loop
    while(gameLooper)
    {
        handleEvents();
        handleFightSequence();
        if(!gameLooper){return 0;};
        printf("Enter a command: ");
        if (fgets(buffer, MAX_INPUT_UNIQUE, stdin) != NULL) {
            //normalize input
            formatInput(buffer);
            //print altered input, unless they hit enter
            if (strcmp(buffer, "") != 0){printf("*You entered: %s\n", buffer);}
            
            //find command
            if (strcmp(buffer, "help") == 0) 
            {
                printf("Commands:\n");
                printf(" - help   -> display this help menu\n");
                printf(" - look   -> describe the current card and reveal items\n");
                printf(" - move   -> change cards\n");
                printf(" - map    -> displays cards with lit fireplaces, that you can jump to\n");
                printf(" - status -> print current status, health, money, mana, etc...\n");
                printf(" - sword  -> use your sword if you have it, s for short\n");
                printf(" - cast   -> cast a spell, not helpful during fights\n");
                printf(" - buy    -> when in a store, you can buy items\n");
                printf(" - jovi   -> your best friend, she's a talking dog\n");
                printf(" - quit   -> quit the program\n");
            }
            else if (strcmp(buffer, "look") == 0)
            {
                printf("\n%s:\n%s\n\n", currentCard->name, currentCard->description);
                for(int i =0; i < currentCard->numLinkedItems; i++)
                {
                    printAndHandleItem(currentCard->linkedItems[i]);
                }
                currentCard->numLinkedItems = 0; //only let the user get the items once
            }
            else if (strcmp(buffer, "move") == 0)
            {
                //print all the places to choose from
                printf("Places to move to: \n");
                for(int i = 0; i<currentCard->numLinkedCards;i++)
                {
                    //print - transition name (we match on that) and card name next to it
                    printf(" - %s - %s\n", currentCard->linkedCards[i].name, deck[currentCard->linkedCards[i].cardType].name);
                }
                //prompt for input to move to
                printf("Where to move?: ");
                if (fgets(buffer, MAX_INPUT_UNIQUE, stdin) != NULL)
                {
                    bool foundMatch = false;
                    //normalize input
                    formatInput(buffer);
                    //search for a match, if match set current card and break
                    for(int i = 0; i<currentCard->numLinkedCards;i++)
                    {
                        if(strcmp(buffer, currentCard->linkedCards[i].name) == 0)
                        {
                            foundMatch = true;
                            currentCard = &deck[currentCard->linkedCards[i].cardType];
                            printf("moved to %s\n", currentCard->name);
                            //if we have alive enemies to fight, on the new card
                            //let the user know whats happening before the fight sequence begins 
                            if(currentCard->numLinkedEnemies > 0)
                            {
                                printf("Enemies here: \n");
                                for(int i = 0; i < currentCard->numLinkedEnemies; i++)
                                {
                                    Enemy enemy = currentCard->linkedEnemies[i];
                                    printf(" - %s ( %d health, does %d damage )\n", getEnemyName(enemy.enemyType), enemy.health, enemy.damage);
                                }
                            }
                            break;
                        }
                    }
                    if(!foundMatch)
                    {
                        printf("(No Move Made)\n");
                    }
                }
                else
                {
                   printf("error getting card to jump to.\n"); 
                }
            }
            else if (strcmp(buffer, "status") == 0)
            {
                printf("Current Status: \n"); 
                printf(" - health       :  %d\n", health);
                printf(" - mana         :  %d\n", mana);
                printf(" - ex_points    :  %d\n", xp);
                printf(" - money        : $%d\n", money);
                printf(" - books        :  %d\n", books);
                if(booksDeliveredToAlistair > 0){printf("  ( %d books delivered to Alistair )\n", booksDeliveredToAlistair);}
                printf(" - loops        :  %d\n", loops);
                if(hasSword){printf(" - you are holding a sword\n");}
                if(hasSword){printf(" - damage       :  %d\n", getDamage());}
                if(hasMap){printf(" - you have a map\n");}
                if(hasRing){printf(" - you have a magic ring\n");}
            }
            else if (strcmp(buffer, "buy") == 0)
            {
                if(currentCard->isStore)
                {
                    if(money > 0)
                    {
                        printf("Items to purchase:\n");
                        printf(" - health - health potion - $10\n");
                        printf(" - mana   - mana   potion - $10\n");
                        printf("Purchase: ");
                        if (fgets(buffer, MAX_INPUT_UNIQUE, stdin) != NULL)
                        {
                            formatInput(buffer);
                            if (strcmp(buffer, "health") == 0)
                            {
                                money -= 10;
                                health += 100;
                                printf("(received 100 health)\n");
                                printf("(cost $10)\n");
                            }
                            else if (strcmp(buffer, "mana") == 0)
                            {
                                money -= 10;
                                mana += 100;
                                printf("(received 100 mana)\n");
                                printf("(cost $10)\n");
                            }
                            else
                            {
                               printf("The store clerk says 'I dont have that item.'\n"); 
                            }
                        }
                        else
                        {
                            printf("Error getting item to purchase.\n");
                        }
                    }
                    else
                    {
                        printf("You are not currently in a store.\n");
                    }
                }
                else
                {
                    printf("You are not currently in a store.\n");
                }
            }
            else if (strcmp(buffer, "sword") == 0 || strcmp(buffer, "s") == 0)
            {
                if(hasSword)
                {
                    printf("You swing the sword and ...\n");
                    for(int i = 0; i < currentCard->numLinkedEnemies; i++)
                    {
                        if(currentCard->linkedEnemies[i].health > 0)
                        {
                            int type = currentCard->linkedEnemies[i].enemyType;
                            int damage = getDamage();
                            printf("You hit the %s with the sword, they lose %d health\n", getEnemyName(type), damage);
                            currentCard->linkedEnemies[i].health-=damage;
                            if(currentCard->linkedEnemies[i].health <= 0)
                            {
                                printf("The %s is dead (gained 10 xp points)\n", getEnemyName(type));
                                xp += 10;
                            }
                            break;
                        }
                        else
                        {
                            continue;
                        }
                    }
                    bool allDead = true;
                    for(int i = 0; i < currentCard->numLinkedEnemies; i++)
                    {
                        if(currentCard->linkedEnemies[i].health > 0)
                        {
                            allDead = false;
                        }
                    }
                    if(allDead)
                    {
                        printf("All enemies have been delt with.\n");
                        currentCard->numLinkedEnemies = 0;
                    }
                }
                else{printf("You do not have the sword.\n");}
            }
            else if (strcmp(buffer, "map") == 0)
            {
                if(!hasMap)
                {
                    printf("You do not have the map yet.\n");
                    continue;
                }
                printf("Places you can jump to: \n");
                if(homeFirePlaceIsLit){printf(" - home\n");}
                if(bellevilleFirePlaceIsLit){printf(" - belleville\n");}
                if(mccannsFirePlaceIsLit){printf(" - mccann\n");}
                if(jeffersFirePlaceIsLit){printf(" - jeffers\n");}
                if(cinderSpireFirePlaceIsLit){printf(" - spire\n");}
                if(williamsFirePlaceIsLit){printf(" - williams\n");}
                if(stackFirePlaceIsLit){printf(" - stack\n");}
                if(villageFirePlaceIsLit){printf(" - village\n");}
                if(ajFirePlaceIsLit){printf(" - aj\n");}
                printf("\n> ");
                if (fgets(buffer, MAX_INPUT_UNIQUE, stdin) != NULL)
                {
                    formatInput(buffer);
                    if (strcmp(buffer, "home") == 0 && homeFirePlaceIsLit)
                    {
                        printf("went home.\n"); 
                        currentCard = &deck[CARD_HOME];
                    }
                    else if (strcmp(buffer, "jeffers") == 0 && jeffersFirePlaceIsLit)
                    {
                        printf("went to old man Jeffers.\n"); 
                        currentCard = &deck[CARD_OLD_MAN_JEFFERS];
                    }
                    else if (strcmp(buffer, "spire") == 0 && cinderSpireFirePlaceIsLit)
                    {
                        printf("went to Cinderspire.\n"); 
                        currentCard = &deck[CARD_CINDERSPIRE];
                    }
                    else if (strcmp(buffer, "mccann") == 0 && mccannsFirePlaceIsLit)
                    {
                        printf("went to old man McCann's.\n"); 
                        currentCard = &deck[CARD_OLD_MAN_MCCANN];
                    }
                    else if (strcmp(buffer, "williams") == 0 && williamsFirePlaceIsLit)
                    {
                        printf("went to Williams Town Hall.\n"); 
                        currentCard = &deck[CARD_WILLIAMS_TOWN_HALL];
                    }
                    else if (strcmp(buffer, "stack") == 0 && stackFirePlaceIsLit)
                    {
                        printf("went to Stack Homestead.\n"); 
                        currentCard = &deck[CARD_STACK_HOMESTEAD];
                    }
                    else if (strcmp(buffer, "village") == 0 && villageFirePlaceIsLit)
                    {
                        printf("went to The Enchanted Village.\n"); 
                        currentCard = &deck[CARD_VILLAGE];
                    }
                    else if (strcmp(buffer, "aj") == 0 && ajFirePlaceIsLit)
                    {
                        printf("went to old man AJ's.\n"); 
                        currentCard = &deck[CARD_AJ];
                    }
                    else if (strcmp(buffer, "belleville") == 0 && bellevilleFirePlaceIsLit)
                    {
                        printf("went to Belleville Public Library.\n"); 
                        currentCard = &deck[CARD_BELLEVILLE_LIBRARY];
                    }
                    else
                    {
                        printf("Thats not on the map.\n"); 
                    }
                }
                else
                {
                    printf("Error reading place to jump to.\n"); 
                }
            }
            else if (strcmp(buffer, "jovi") == 0)
            {
                printf("Talk to Jovi: ");
                if (fgets(buffer, MAX_INPUT_UNIQUE, stdin) != NULL)
                {
                    formatInput(buffer);
                    handleJovi(buffer);
                }
                else
                {
                    printf("Error reading text for Jovi.\n"); 
                }
            }
            else if (strcmp(buffer, "cheat") == 0 && cheating)
            {
                printf("...dirty cheater...\n");
                //prompt for input to move to
                printf("> ");
                if (fgets(buffer, MAX_INPUT_UNIQUE, stdin) != NULL)
                {
                    //normalize input
                    formatInput(buffer);
                    //match cheat code
                    if (strcmp(buffer, "give me money") == 0)
                    {
                        printf("received $400\n");
                        money+=400;
                    }
                    else if (strcmp(buffer, "xp") == 0)
                    {
                        printf("received 100 xp\n");
                        xp+=100;
                    }
                    else if (strcmp(buffer, "mana") == 0)
                    {
                        printf("received 100 mana\n");
                        xp+=100;
                    }
                    else if (strcmp(buffer, "to be young") == 0)
                    {
                        printf("received 100 health\n");
                        health+=100;
                    }
                    else if (strcmp(buffer, "map") == 0)
                    {
                        printf("Got the Map\n");
                        hasMap = true;
                    }
                    else if (strcmp(buffer, "loop de loop") == 0)
                    {
                        printf("gameloop counter incremented by 100\n");
                        loops+=100;
                    }
                    else if (strcmp(buffer, "devel") == 0)
                    {
                        printf("developer mode\n");
                        //this cheat makes it so you can jump to any card with a lot of health and all the things
                        money=100000;
                        loops=100000;
                        health=100000;
                        mana=100000;
                        xp=100000;
                        hasRing = true;
                        hasSword = true;
                        hasMap = true;
                        homeFirePlaceIsLit = true;
                        jeffersFirePlaceIsLit = true;
                        cinderSpireFirePlaceIsLit = true;
                        mccannsFirePlaceIsLit = true;
                        williamsFirePlaceIsLit = true;
                        stackFirePlaceIsLit = true;
                        villageFirePlaceIsLit = true;
                        ajFirePlaceIsLit = true;
                        bellevilleFirePlaceIsLit = true;
                    }
                    else if (strcmp(buffer, "goto") == 0)
                    {
                        int userInput;
                        printf("All Cards: \n");
                        for(int i=0; i < TotalNumberOfCards; i++)
                        {
                            printf(" - %d - %s\n",deck[i].cardType,deck[i].name);
                        }
                        printf("Card: ");
                        scanf("%d", &userInput);
                        if(userInput >= 0 && userInput < TotalNumberOfCards)
                        {
                            currentCard = &deck[userInput];
                            printf("jumped to %s\n", deck[userInput].name);
                        }
                        else
                        {
                            printf("That was not a valid card number.\n");
                        }
                    }
                }
                else
                {
                   printf("Error reading cheat code.\n"); 
                }
            }
            else if (strcmp(buffer, "cast") == 0)
            {
                if(hasRing)
                {
                    printf("Spellbook:\n");
                    printf(" - fire\n");
                    printf(" - grow\n");
                    printf(" - open\n");
                    printf("\nSpell: ");
                    if (fgets(buffer, MAX_INPUT_UNIQUE, stdin) != NULL)
                    {
                        formatInput(buffer);
                        handleCast(buffer);
                    }
                    else
                    {
                        printf("Error reading spell.\n"); 
                    }
                }
                else
                {
                    printf("Cannot cast spells without a magical object...\n"); 
                }
            }
            else if (strcmp(buffer, "quit") == 0 || strcmp(buffer, "q") == 0 || strcmp(buffer, "exit") == 0 || strcmp(buffer, "x") == 0)
            {
                printf("Good Bye!\n");
                return 0;
            }
            else if (strcmp(buffer, "") == 0){/* keep from poluting the screen */}
            /* more else if clauses */
            else /* default: */
            {
                printf("I dont recognize that command. (type help for help)\n");
            }
        } else {
            printf("Error reading input.\n");
            return 0;
        }
        loops++;//increment game loop counter
    }

    //exit if game loop is turned off
    return 0;
}


void initDeck() {
    // Card 1 (Starting Room) your home - has event, has fireplace
    deck[CARD_HOME].cardType = CARD_HOME;
    deck[CARD_HOME].numLinkedCards = 2;
    deck[CARD_HOME].numLinkedItems = 4;
    strcpy(deck[CARD_HOME].name, "home");
    strcpy(deck[CARD_HOME].description, "You are in your home.\nIts dimly lit but cozy.\nThe light from an old lamp beside a cozy armchair pools softly over a stack of books,\ninviting anyone to curl up with a cup of tea or coffee.\nEvery corner feels like a sanctuary, a quiet world where time slows down,\nand every object seems to have its own story to tell.\nThere is a front door and a back door.\nThere is a fire place here.");
    deck[CARD_HOME].linkedCards[0].cardType = CARD_FRONTDOOR; //link to front door
    strcpy(deck[CARD_HOME].linkedCards[0].name,"front"); //frontdoor link name
    deck[CARD_HOME].linkedCards[1].cardType = CARD_BACKDOOR; //link to back door / back yard
    strcpy(deck[CARD_HOME].linkedCards[1].name,"back"); //frontdoor link name
    deck[CARD_HOME].linkedItems[0] = ITEM_SWORD; //sword
    deck[CARD_HOME].linkedItems[1] = ITEM_BOOK; //sword
    deck[CARD_HOME].linkedItems[2] = ITEM_HEALTH_POTION; //potion
    deck[CARD_HOME].linkedItems[3] = ITEM_MANA_POTION; //potion

    // Card 2  - outside the front door
    deck[CARD_FRONTDOOR].cardType = CARD_FRONTDOOR;
    deck[CARD_FRONTDOOR].numLinkedCards = 3;
    strcpy(deck[CARD_FRONTDOOR].name, "Your Frontdoor");
    strcpy(deck[CARD_FRONTDOOR].description, "You are standing just outside your front door.\nThere is a dirt road.\nYou can go left or right.");
    deck[CARD_FRONTDOOR].linkedCards[0].cardType = CARD_HOME; //link back to home
    strcpy(deck[CARD_FRONTDOOR].linkedCards[0].name,"home"); //link name
    deck[CARD_FRONTDOOR].linkedCards[1].cardType = CARD_FIRST_ROAD_LEFT; //link to left down the road
    strcpy(deck[CARD_FRONTDOOR].linkedCards[1].name,"left"); //link name
    deck[CARD_FRONTDOOR].linkedCards[2].cardType = CARD_FIRST_ROAD_RIGHT; //link to right down the road
    strcpy(deck[CARD_FRONTDOOR].linkedCards[2].name,"right"); //link name

    // Card 3  - left on the road outside your house
    deck[CARD_FIRST_ROAD_LEFT].cardType = CARD_FIRST_ROAD_LEFT;
    deck[CARD_FIRST_ROAD_LEFT].numLinkedCards = 2;
    strcpy(deck[CARD_FIRST_ROAD_LEFT].name, "Far down the Road");
    strcpy(deck[CARD_FIRST_ROAD_LEFT].description, "You move left down the road until you come to a large tree.\nThe tree is very big and very old.");
    deck[CARD_FIRST_ROAD_LEFT].linkedCards[0].cardType = CARD_FRONTDOOR; //link to front door
    strcpy(deck[CARD_FIRST_ROAD_LEFT].linkedCards[0].name,"front"); //link name
    deck[CARD_FIRST_ROAD_LEFT].linkedCards[1].cardType = CARD_FIRST_CLIMB_TREE; //link to climable tree
    strcpy(deck[CARD_FIRST_ROAD_LEFT].linkedCards[1].name,"tree"); //link name

    // Card 4  - right on the road outside your house
    deck[CARD_FIRST_ROAD_RIGHT].cardType = CARD_FIRST_ROAD_RIGHT;
    deck[CARD_FIRST_ROAD_RIGHT].numLinkedCards = 2;
    strcpy(deck[CARD_FIRST_ROAD_RIGHT].name, "Up the Road");
    strcpy(deck[CARD_FIRST_ROAD_RIGHT].description, "You move right down the road.\nLooks like trouble up ahead...");
    deck[CARD_FIRST_ROAD_RIGHT].linkedCards[0].cardType = CARD_FRONTDOOR; //link to front door
    strcpy(deck[CARD_FIRST_ROAD_RIGHT].linkedCards[0].name,"front"); //link name
    deck[CARD_FIRST_ROAD_RIGHT].linkedCards[1].cardType = CARD_FIRST_TROUBLE; //first trouble
    strcpy(deck[CARD_FIRST_ROAD_RIGHT].linkedCards[1].name,"trouble"); //link name

    // Card 5  - climb-able tree
    deck[CARD_FIRST_CLIMB_TREE].cardType = CARD_FIRST_CLIMB_TREE;
    deck[CARD_FIRST_CLIMB_TREE].numLinkedCards = 2;
    deck[CARD_FIRST_CLIMB_TREE].numLinkedItems = 2;
    strcpy(deck[CARD_FIRST_CLIMB_TREE].name, "Up in the Tree");
    strcpy(deck[CARD_FIRST_CLIMB_TREE].description, "You climb the tree.\nYou can see everything from up here.");
    deck[CARD_FIRST_CLIMB_TREE].linkedCards[0].cardType = CARD_DEAD_END; //link to first left road path
    strcpy(deck[CARD_FIRST_CLIMB_TREE].linkedCards[0].name,"down"); //link name
    deck[CARD_FIRST_CLIMB_TREE].linkedCards[1].cardType = CARD_FIRST_ROAD_LEFT; //link to first left road path
    strcpy(deck[CARD_FIRST_CLIMB_TREE].linkedCards[1].name,"back"); //link name
    deck[CARD_FIRST_CLIMB_TREE].linkedItems[0] = ITEM_MAP; //map
    deck[CARD_FIRST_CLIMB_TREE].linkedItems[1] = ITEM_COINS; //some coins

    // Card 6  - further down the road to the right
    deck[CARD_FIRST_TROUBLE].cardType = CARD_FIRST_TROUBLE;
    deck[CARD_FIRST_TROUBLE].numLinkedCards = 2;
    deck[CARD_FIRST_TROUBLE].numLinkedEnemies = 2;
    strcpy(deck[CARD_FIRST_TROUBLE].name, "Clearing along the Road");
    strcpy(deck[CARD_FIRST_TROUBLE].description, "You are in a clearing along the road.\nThere is a lot of open space.");
    deck[CARD_FIRST_TROUBLE].linkedCards[0].cardType = CARD_FIRST_ROAD_RIGHT; //back closer to home
    strcpy(deck[CARD_FIRST_TROUBLE].linkedCards[0].name,"back"); //link name
    deck[CARD_FIRST_TROUBLE].linkedCards[1].cardType = CARD_ROAD_AFTER_TROUBLE; //long stretch of road
    strcpy(deck[CARD_FIRST_TROUBLE].linkedCards[1].name,"road"); //link name
    deck[CARD_FIRST_TROUBLE].linkedEnemies[0].enemyType = ENEMY_OGRE; //ogre
    deck[CARD_FIRST_TROUBLE].linkedEnemies[0].health = 30; //ogre
    deck[CARD_FIRST_TROUBLE].linkedEnemies[0].damage = 10; //ogre
    deck[CARD_FIRST_TROUBLE].linkedEnemies[1].enemyType = ENEMY_WOLF; //wolf
    deck[CARD_FIRST_TROUBLE].linkedEnemies[1].health = 10; //wolf
    deck[CARD_FIRST_TROUBLE].linkedEnemies[1].damage = 5; //wolf

    // Card 7  - dead end after the tree - has event
    deck[CARD_DEAD_END].cardType = CARD_DEAD_END;
    deck[CARD_DEAD_END].numLinkedCards = 3;
    strcpy(deck[CARD_DEAD_END].name, "Dead End after the Tree");
    strcpy(deck[CARD_DEAD_END].description, "The road comes to a dead end after the tree.\nBut there is a path.\nYou could also follow the squirrel into the woods.");
    deck[CARD_DEAD_END].linkedCards[0].cardType = CARD_FIRST_PATH; //link to first path
    strcpy(deck[CARD_DEAD_END].linkedCards[0].name,"path"); //link name
    deck[CARD_DEAD_END].linkedCards[1].cardType = CARD_FIRST_CLIMB_TREE; //back to tree
    strcpy(deck[CARD_DEAD_END].linkedCards[1].name,"tree"); //link name
    deck[CARD_DEAD_END].linkedCards[2].cardType = CARD_LOST_WOODS1; //begin lost in the woods sequence, leads to wrenwood
    strcpy(deck[CARD_DEAD_END].linkedCards[2].name,"wood"); //link name

    // Card 8  - path to old man jeffers
    deck[CARD_FIRST_PATH].cardType = CARD_FIRST_PATH;
    deck[CARD_FIRST_PATH].numLinkedCards = 3;
    strcpy(deck[CARD_FIRST_PATH].name, "Path in the Woods");
    strcpy(deck[CARD_FIRST_PATH].description, "You are on a small path in the woods\nIt looks like there is a small cottage up ahead.");
    deck[CARD_FIRST_PATH].linkedCards[0].cardType = CARD_DEAD_END; //link back to dead end
    strcpy(deck[CARD_FIRST_PATH].linkedCards[0].name,"dead"); //link name
    deck[CARD_FIRST_PATH].linkedCards[1].cardType = CARD_ROAD_AFTER_TROUBLE; //link back to road
    strcpy(deck[CARD_FIRST_PATH].linkedCards[1].name,"road"); //link name
    deck[CARD_FIRST_PATH].linkedCards[2].cardType = CARD_OLD_MAN_JEFFERS; //link to old man jeffers
    strcpy(deck[CARD_FIRST_PATH].linkedCards[2].name,"jeffers"); //link name

    // Card 9  - long stretch of road after trouble clearing
    deck[CARD_ROAD_AFTER_TROUBLE].cardType = CARD_ROAD_AFTER_TROUBLE;
    deck[CARD_ROAD_AFTER_TROUBLE].numLinkedCards = 3;
    strcpy(deck[CARD_ROAD_AFTER_TROUBLE].name, "Long Stretch of Road near town");
    strcpy(deck[CARD_ROAD_AFTER_TROUBLE].description, "You are on a long stretch of road.\nYou are near town.\nThere is also a small path.");
    deck[CARD_ROAD_AFTER_TROUBLE].linkedCards[0].cardType = CARD_FIRST_PATH; //link to first path
    strcpy(deck[CARD_ROAD_AFTER_TROUBLE].linkedCards[0].name,"path"); //link name
    deck[CARD_ROAD_AFTER_TROUBLE].linkedCards[1].cardType = CARD_FIRST_TROUBLE; //back to trouble
    strcpy(deck[CARD_ROAD_AFTER_TROUBLE].linkedCards[1].name,"trouble"); //link name
    deck[CARD_ROAD_AFTER_TROUBLE].linkedCards[2].cardType = CARD_TOWN_SQUARE; //town square
    strcpy(deck[CARD_ROAD_AFTER_TROUBLE].linkedCards[2].name,"town"); //link name

    // Card 10  - old man jeffers - has fireplace
    deck[CARD_OLD_MAN_JEFFERS].cardType = CARD_OLD_MAN_JEFFERS;
    deck[CARD_OLD_MAN_JEFFERS].numLinkedCards = 2;
    deck[CARD_OLD_MAN_JEFFERS].numLinkedItems = 2;
    strcpy(deck[CARD_OLD_MAN_JEFFERS].name, "Old Man Jeffers");
    strcpy(deck[CARD_OLD_MAN_JEFFERS].description, "You come across old man Jeffers residence.\nHe has a garden and a fireplace.");
    deck[CARD_OLD_MAN_JEFFERS].linkedCards[0].cardType = CARD_FIRST_PATH; //link back to path
    strcpy(deck[CARD_OLD_MAN_JEFFERS].linkedCards[0].name,"path"); //link name
    deck[CARD_OLD_MAN_JEFFERS].linkedCards[1].cardType = CARD_JEFFERS_GARDEN; //link to garden
    strcpy(deck[CARD_OLD_MAN_JEFFERS].linkedCards[1].name,"garden"); //link name
    deck[CARD_OLD_MAN_JEFFERS].linkedItems[0] = ITEM_MANA_POTION; //mana
    deck[CARD_OLD_MAN_JEFFERS].linkedItems[1] = ITEM_COINS; //some coins

    // Card 11  - jeffers garden, good place to get health, has grow
    deck[CARD_JEFFERS_GARDEN].cardType = CARD_JEFFERS_GARDEN;
    deck[CARD_JEFFERS_GARDEN].numLinkedCards = 1;
    deck[CARD_JEFFERS_GARDEN].numLinkedItems = 2;
    strcpy(deck[CARD_JEFFERS_GARDEN].name, "Old Man Jeffers Garden");
    strcpy(deck[CARD_JEFFERS_GARDEN].description, "You are in old man Jeffers garden.\nThere is a tomatoe plant.");
    deck[CARD_JEFFERS_GARDEN].linkedCards[0].cardType = CARD_OLD_MAN_JEFFERS; //link to first path
    strcpy(deck[CARD_JEFFERS_GARDEN].linkedCards[0].name,"jeffers"); //link name
    deck[CARD_JEFFERS_GARDEN].linkedItems[0] = ITEM_HEALTH_POTION; //health
    deck[CARD_JEFFERS_GARDEN].linkedItems[1] = ITEM_HEALTH_POTION; //health

    // Card 12  - town square - has event
    deck[CARD_TOWN_SQUARE].cardType = CARD_TOWN_SQUARE;
    deck[CARD_TOWN_SQUARE].numLinkedCards = 5;
    deck[CARD_TOWN_SQUARE].numLinkedItems = 6;
    strcpy(deck[CARD_TOWN_SQUARE].name, "Town Square");
    strcpy(deck[CARD_TOWN_SQUARE].description, "You are in the town square.\nThere is a store here and a few paths.");
    deck[CARD_TOWN_SQUARE].linkedCards[0].cardType = CARD_ROAD_AFTER_TROUBLE; //link to first path
    strcpy(deck[CARD_TOWN_SQUARE].linkedCards[0].name,"road"); //link name
    deck[CARD_TOWN_SQUARE].linkedCards[1].cardType = CARD_FIRST_STORE; //link to store
    strcpy(deck[CARD_TOWN_SQUARE].linkedCards[1].name,"store"); //link name
    deck[CARD_TOWN_SQUARE].linkedCards[2].cardType = CARD_MIRKWOOD_ENTER; //link to mirkwood enter
    strcpy(deck[CARD_TOWN_SQUARE].linkedCards[2].name,"wood"); //link name
    deck[CARD_TOWN_SQUARE].linkedCards[3].cardType = CARD_CAVE_ENTER; //link to cave enter
    strcpy(deck[CARD_TOWN_SQUARE].linkedCards[3].name,"cave"); //link name
    deck[CARD_TOWN_SQUARE].linkedCards[4].cardType = CARD_TOWN_HOUSE; //link to cave enter
    strcpy(deck[CARD_TOWN_SQUARE].linkedCards[4].name,"house"); //link name
    deck[CARD_TOWN_SQUARE].linkedItems[0] = ITEM_HEALTH_POTION; //health
    deck[CARD_TOWN_SQUARE].linkedItems[1] = ITEM_MANA_POTION; //mana
    deck[CARD_TOWN_SQUARE].linkedItems[2] = ITEM_HEALTH_POTION; //health
    deck[CARD_TOWN_SQUARE].linkedItems[3] = ITEM_MANA_POTION; //mana
    deck[CARD_TOWN_SQUARE].linkedItems[4] = ITEM_COINS; //some coins
    deck[CARD_TOWN_SQUARE].linkedItems[5] = ITEM_COINS; //some coins

    // Card 13  - first store
    deck[CARD_FIRST_STORE].cardType = CARD_FIRST_STORE;
    deck[CARD_FIRST_STORE].isStore = true;
    deck[CARD_FIRST_STORE].numLinkedCards = 1;
    strcpy(deck[CARD_FIRST_STORE].name, "Mom and Pop Shop");
    strcpy(deck[CARD_FIRST_STORE].description, "You are in a small store.\nBuy items.");
    deck[CARD_FIRST_STORE].linkedCards[0].cardType = CARD_TOWN_SQUARE; //link back to town
    strcpy(deck[CARD_FIRST_STORE].linkedCards[0].name,"town"); //link name

    // Card 14  - mirkwood entrance
    deck[CARD_MIRKWOOD_ENTER].cardType = CARD_MIRKWOOD_ENTER;
    deck[CARD_MIRKWOOD_ENTER].numLinkedCards = 2;
    strcpy(deck[CARD_MIRKWOOD_ENTER].name, "Mirkwood Entrance");
    strcpy(deck[CARD_MIRKWOOD_ENTER].description, "You take a small path to the entrance of Mirkwood forest.\nAll the trees seem to be dead.\nThe air is heavy with mist,\nand the silence is broken only by the occasional rustle of unseen creatures.\nA sense of unease lingers,\nas if the forest itself watches those who enter.");
    deck[CARD_MIRKWOOD_ENTER].linkedCards[0].cardType = CARD_TOWN_SQUARE; //link back to town
    strcpy(deck[CARD_MIRKWOOD_ENTER].linkedCards[0].name,"town"); //link name
    deck[CARD_MIRKWOOD_ENTER].linkedCards[1].cardType = CARD_MIRKWOOD_1; //link to mirkwood 1
    strcpy(deck[CARD_MIRKWOOD_ENTER].linkedCards[1].name,"wood"); //link name

    // Card 15  - cave entrance
    deck[CARD_CAVE_ENTER].cardType = CARD_CAVE_ENTER;
    deck[CARD_CAVE_ENTER].numLinkedCards = 2;
    strcpy(deck[CARD_CAVE_ENTER].name, "Cave Entrance");
    strcpy(deck[CARD_CAVE_ENTER].description, "You take a path to a cave.\nYou are standing at the entrance of the cave.");
    deck[CARD_CAVE_ENTER].linkedCards[0].cardType = CARD_TOWN_SQUARE; //link back to town
    strcpy(deck[CARD_CAVE_ENTER].linkedCards[0].name,"town"); //link name
    deck[CARD_CAVE_ENTER].linkedCards[1].cardType = CARD_CAVE_MIDDLE; //link to middle of cave
    strcpy(deck[CARD_CAVE_ENTER].linkedCards[1].name,"cave"); //link name

    // Card 16  - cave middle
    deck[CARD_CAVE_MIDDLE].cardType = CARD_CAVE_MIDDLE;
    deck[CARD_CAVE_MIDDLE].numLinkedCards = 2;
    strcpy(deck[CARD_CAVE_MIDDLE].name, "Middle of the Cave");
    strcpy(deck[CARD_CAVE_MIDDLE].description, "You are in a cave.");
    deck[CARD_CAVE_MIDDLE].linkedCards[0].cardType = CARD_CAVE_ENTER; //link back to entrance
    strcpy(deck[CARD_CAVE_MIDDLE].linkedCards[0].name,"out"); //link name
    deck[CARD_CAVE_MIDDLE].linkedCards[1].cardType = CARD_CAVE_DEEP; //link to deep cave
    strcpy(deck[CARD_CAVE_MIDDLE].linkedCards[1].name,"cave"); //link name

    // Card 17  - cave deep
    deck[CARD_CAVE_DEEP].cardType = CARD_CAVE_DEEP;
    deck[CARD_CAVE_DEEP].numLinkedCards = 1;
    deck[CARD_CAVE_DEEP].numLinkedItems = 6;
    strcpy(deck[CARD_CAVE_DEEP].name, "Deep in the Cave");
    strcpy(deck[CARD_CAVE_DEEP].description, "You are deep inside a cave.\nThere is a door here but it is locked.");
    deck[CARD_CAVE_DEEP].linkedCards[0].cardType = CARD_CAVE_MIDDLE; //link back to middle
    strcpy(deck[CARD_CAVE_DEEP].linkedCards[0].name,"out"); //link name
    deck[CARD_CAVE_DEEP].linkedItems[0] = ITEM_COINS; //some coins
    deck[CARD_CAVE_DEEP].linkedItems[1] = ITEM_COINS; //some coins
    deck[CARD_CAVE_DEEP].linkedItems[2] = ITEM_COINS; //some coins
    deck[CARD_CAVE_DEEP].linkedItems[3] = ITEM_COINS; //some coins
    deck[CARD_CAVE_DEEP].linkedItems[4] = ITEM_COINS; //some coins
    deck[CARD_CAVE_DEEP].linkedItems[5] = ITEM_COINS; //some coins

    // Card 18  - Mirkwood 1
    deck[CARD_MIRKWOOD_1].cardType = CARD_MIRKWOOD_1;
    deck[CARD_MIRKWOOD_1].numLinkedCards = 2;
    deck[CARD_MIRKWOOD_1].numLinkedEnemies = 1;
    strcpy(deck[CARD_MIRKWOOD_1].name, "Mirkwood treeline");
    strcpy(deck[CARD_MIRKWOOD_1].description, "You are on a path through Mirkwood.\nThe path is long.");
    deck[CARD_MIRKWOOD_1].linkedCards[0].cardType = CARD_MIRKWOOD_ENTER; //link back to entrance
    strcpy(deck[CARD_MIRKWOOD_1].linkedCards[0].name,"out"); //link name
    deck[CARD_MIRKWOOD_1].linkedCards[1].cardType = CARD_MIRKWOOD_2; //link to mirkwood 2
    strcpy(deck[CARD_MIRKWOOD_1].linkedCards[1].name,"wood"); //link name
    deck[CARD_MIRKWOOD_1].linkedEnemies[0].enemyType = ENEMY_GOBLIN; //goblin
    deck[CARD_MIRKWOOD_1].linkedEnemies[0].health = 10; //goblin
    deck[CARD_MIRKWOOD_1].linkedEnemies[0].damage = 10; //goblin


    // Card 19  - Mirkwood 2
    deck[CARD_MIRKWOOD_2].cardType = CARD_MIRKWOOD_2;
    deck[CARD_MIRKWOOD_2].numLinkedCards = 2;
    deck[CARD_MIRKWOOD_2].numLinkedEnemies = 3;
    strcpy(deck[CARD_MIRKWOOD_2].name, "Mirkwood path");
    strcpy(deck[CARD_MIRKWOOD_2].description, "You are on a path through Mirkwood.\nThere is a clearing up ahead.");
    deck[CARD_MIRKWOOD_2].linkedCards[0].cardType = CARD_MIRKWOOD_1; //link back to mirkwood 1
    strcpy(deck[CARD_MIRKWOOD_2].linkedCards[0].name,"out"); //link name
    deck[CARD_MIRKWOOD_2].linkedCards[1].cardType = CARD_MIRKWOOD_3; //link to mirkwood 3
    strcpy(deck[CARD_MIRKWOOD_2].linkedCards[1].name,"wood"); //link name
    deck[CARD_MIRKWOOD_2].linkedEnemies[0].enemyType = ENEMY_GOBLIN; //goblin
    deck[CARD_MIRKWOOD_2].linkedEnemies[0].health = 30; //goblin
    deck[CARD_MIRKWOOD_2].linkedEnemies[0].damage = 10; //goblin
    deck[CARD_MIRKWOOD_2].linkedEnemies[1].enemyType = ENEMY_GOBLIN; //goblin
    deck[CARD_MIRKWOOD_2].linkedEnemies[1].health = 20; //goblin
    deck[CARD_MIRKWOOD_2].linkedEnemies[1].damage = 20; //goblin
    deck[CARD_MIRKWOOD_2].linkedEnemies[2].enemyType = ENEMY_GOBLIN; //goblin
    deck[CARD_MIRKWOOD_2].linkedEnemies[2].health = 10; //goblin
    deck[CARD_MIRKWOOD_2].linkedEnemies[2].damage = 30; //goblin

    // Card 20  - Mirkwood 3
    deck[CARD_MIRKWOOD_3].cardType = CARD_MIRKWOOD_3;
    deck[CARD_MIRKWOOD_3].numLinkedCards = 2;
    deck[CARD_MIRKWOOD_3].numLinkedEnemies = 4;
    strcpy(deck[CARD_MIRKWOOD_3].name, "Mirkwood clearing");
    strcpy(deck[CARD_MIRKWOOD_3].description, "You are in a clearing in Mirkwood.\nThe exit is nearby.");
    deck[CARD_MIRKWOOD_3].linkedCards[0].cardType = CARD_MIRKWOOD_2; //link back to mirkwood 2
    strcpy(deck[CARD_MIRKWOOD_3].linkedCards[0].name,"out"); //link name
    deck[CARD_MIRKWOOD_3].linkedCards[1].cardType = CARD_MIRKWOOD_EXIT; //link to mirkwood exit
    strcpy(deck[CARD_MIRKWOOD_3].linkedCards[1].name,"wood"); //link name
    deck[CARD_MIRKWOOD_3].linkedEnemies[0].enemyType = ENEMY_BEAR; //bear
    deck[CARD_MIRKWOOD_3].linkedEnemies[0].health = 40; 
    deck[CARD_MIRKWOOD_3].linkedEnemies[0].damage = 20; 
    deck[CARD_MIRKWOOD_3].linkedEnemies[1].enemyType = ENEMY_GOBLIN; //goblin
    deck[CARD_MIRKWOOD_3].linkedEnemies[1].health = 20; 
    deck[CARD_MIRKWOOD_3].linkedEnemies[1].damage = 20; 
    deck[CARD_MIRKWOOD_3].linkedEnemies[2].enemyType = ENEMY_WOLF; //wolf
    deck[CARD_MIRKWOOD_3].linkedEnemies[2].health = 10; 
    deck[CARD_MIRKWOOD_3].linkedEnemies[2].damage = 10; 
    deck[CARD_MIRKWOOD_3].linkedEnemies[3].enemyType = ENEMY_ORC; //orc...uh oh
    deck[CARD_MIRKWOOD_3].linkedEnemies[3].health = 100; 
    deck[CARD_MIRKWOOD_3].linkedEnemies[3].damage = 40; 

    // Card 21  - Mirkwood Exit
    deck[CARD_MIRKWOOD_EXIT].cardType = CARD_MIRKWOOD_EXIT;
    deck[CARD_MIRKWOOD_EXIT].numLinkedCards = 5;
    deck[CARD_MIRKWOOD_EXIT].numLinkedItems = 4;
    strcpy(deck[CARD_MIRKWOOD_EXIT].name, "Mirkwood exit");
    strcpy(deck[CARD_MIRKWOOD_EXIT].description, "You are just outside Mirkwood.\nThere is a road sign with many directions.\nYou can take many paths here.");
    deck[CARD_MIRKWOOD_EXIT].linkedCards[0].cardType = CARD_MIRKWOOD_3; //link back to mirkwood 3
    strcpy(deck[CARD_MIRKWOOD_EXIT].linkedCards[0].name,"wood"); //link name
    deck[CARD_MIRKWOOD_EXIT].linkedCards[1].cardType = CARD_BRAMBLETHORN_OVERLOOK; //link Bramblethorn Overlook
    strcpy(deck[CARD_MIRKWOOD_EXIT].linkedCards[1].name,"overlook"); //link name
    deck[CARD_MIRKWOOD_EXIT].linkedCards[2].cardType = CARD_IRONWOOD_CLEARING; //link Ironwood Clearing
    strcpy(deck[CARD_MIRKWOOD_EXIT].linkedCards[2].name,"clearing"); //link name
    deck[CARD_MIRKWOOD_EXIT].linkedCards[3].cardType = CARD_ECHOING_HOLLOW; //link Echoing Hollow
    strcpy(deck[CARD_MIRKWOOD_EXIT].linkedCards[3].name,"hollow"); //link name
    deck[CARD_MIRKWOOD_EXIT].linkedCards[4].cardType = CARD_CINDERSPIRE; //link Cinderspire
    strcpy(deck[CARD_MIRKWOOD_EXIT].linkedCards[4].name,"spire"); //link name
    deck[CARD_MIRKWOOD_EXIT].linkedItems[0] = ITEM_HEALTH_POTION; //health
    deck[CARD_MIRKWOOD_EXIT].linkedItems[1] = ITEM_HEALTH_POTION; //health
    deck[CARD_MIRKWOOD_EXIT].linkedItems[2] = ITEM_HEALTH_POTION; //health
    deck[CARD_MIRKWOOD_EXIT].linkedItems[3] = ITEM_HEALTH_POTION; //health

    // Card 22  - town house 1 - has grow
    deck[CARD_TOWN_HOUSE].cardType = CARD_TOWN_HOUSE;
    deck[CARD_TOWN_HOUSE].numLinkedCards = 1;
    strcpy(deck[CARD_TOWN_HOUSE].name, "Small Wooden House");
    strcpy(deck[CARD_TOWN_HOUSE].description, "You stand before a small wooden house,\nits weathered planks bearing the marks of time and countless storms.\nThe roof,\npatched with moss and stray shingles,\nslopes gently, giving the structure a quaint, inviting charm.\nThere is a woman here.");
    deck[CARD_TOWN_HOUSE].linkedCards[0].cardType = CARD_TOWN_SQUARE; //link back to town
    strcpy(deck[CARD_TOWN_HOUSE].linkedCards[0].name,"town"); //link name

    // Card 23  - backdoor/backyard
    deck[CARD_BACKDOOR].cardType = CARD_BACKDOOR;
    deck[CARD_BACKDOOR].numLinkedCards = 2;
    deck[CARD_BACKDOOR].numLinkedItems = 1;
    strcpy(deck[CARD_BACKDOOR].name, "Your Backyard");
    strcpy(deck[CARD_BACKDOOR].description, "You are in your back yard.\nThere is a path into the woods.");
    deck[CARD_BACKDOOR].linkedCards[0].cardType = CARD_HOME; //link back to home
    strcpy(deck[CARD_BACKDOOR].linkedCards[0].name,"home"); //link name
    deck[CARD_BACKDOOR].linkedCards[1].cardType = CARD_SECOND_PATH; //link to path
    strcpy(deck[CARD_BACKDOOR].linkedCards[1].name,"path"); //link name
    deck[CARD_BACKDOOR].linkedItems[0] = ITEM_MANA_POTION; //potion

    // Card 24  - second path, backyard path into the woods
    deck[CARD_SECOND_PATH].cardType = CARD_SECOND_PATH;
    deck[CARD_SECOND_PATH].numLinkedCards = 2;
    strcpy(deck[CARD_SECOND_PATH].name, "Path in the Woods");
    strcpy(deck[CARD_SECOND_PATH].description, "You take the path deep into the woods.\nThe path continues to a garden.\nThere is also a deep river nearby. Too deep to cross to the other side...");
    deck[CARD_SECOND_PATH].linkedCards[0].cardType = CARD_BACKDOOR; //link back to backyard/backdoor
    strcpy(deck[CARD_SECOND_PATH].linkedCards[0].name,"back"); //link name
    deck[CARD_SECOND_PATH].linkedCards[1].cardType = CARD_WILD_GARDEN; //link to wild garden
    strcpy(deck[CARD_SECOND_PATH].linkedCards[1].name,"garden"); //link name

    // Card 25  - wild garden - has event, has grow, has locked shed entrance to CARD_FIRST_SHED
    deck[CARD_WILD_GARDEN].cardType = CARD_WILD_GARDEN;
    deck[CARD_WILD_GARDEN].numLinkedCards = 1;
    strcpy(deck[CARD_WILD_GARDEN].name, "Wild Garden");
    strcpy(deck[CARD_WILD_GARDEN].description, "You are in a wild garden with lots of plants to choose from.\nThere is an apple tree here.\nThere is a shed nearby but the door is locked.");
    deck[CARD_WILD_GARDEN].linkedCards[0].cardType = CARD_SECOND_PATH; //link to second path
    strcpy(deck[CARD_WILD_GARDEN].linkedCards[0].name,"path"); //link name

    // Card 26  - Bramblethorn Overlook
    deck[CARD_BRAMBLETHORN_OVERLOOK].cardType = CARD_BRAMBLETHORN_OVERLOOK;
    deck[CARD_BRAMBLETHORN_OVERLOOK].numLinkedCards = 2;
    strcpy(deck[CARD_BRAMBLETHORN_OVERLOOK].name, "Bramblethorn Overlook");
    strcpy(deck[CARD_BRAMBLETHORN_OVERLOOK].description, "The view is quite impressive.\nYou can see out over much of the forest and a river.\nThere is a bridge nearby.");
    deck[CARD_BRAMBLETHORN_OVERLOOK].linkedCards[0].cardType = CARD_MIRKWOOD_EXIT; //link back to mirkwood exit
    strcpy(deck[CARD_BRAMBLETHORN_OVERLOOK].linkedCards[0].name,"wood"); //link name
    deck[CARD_BRAMBLETHORN_OVERLOOK].linkedCards[1].cardType = CARD_FIRST_BRIDGE; //link to bridge
    strcpy(deck[CARD_BRAMBLETHORN_OVERLOOK].linkedCards[1].name,"bridge"); //link name
    
    // Card 27  - Ironwood Clearing
    deck[CARD_IRONWOOD_CLEARING].cardType = CARD_IRONWOOD_CLEARING;
    deck[CARD_IRONWOOD_CLEARING].numLinkedCards = 1;
    deck[CARD_IRONWOOD_CLEARING].numLinkedEnemies = 2;
    strcpy(deck[CARD_IRONWOOD_CLEARING].name, "Ironwood Clearing");
    strcpy(deck[CARD_IRONWOOD_CLEARING].description, "The road opens up into a rare, serene clearing.\nThere is a lot of space.\nThe air is heavy with a quiet power, a feeling that this place is not entirely of the natural world.\nThere is a small shed nearby, but the door is locked.");
    deck[CARD_IRONWOOD_CLEARING].linkedCards[0].cardType = CARD_MIRKWOOD_EXIT; //link back to mirkwood exit
    strcpy(deck[CARD_IRONWOOD_CLEARING].linkedCards[0].name,"wood"); //link name
    deck[CARD_IRONWOOD_CLEARING].linkedEnemies[0].enemyType = ENEMY_BEAR; //bear
    deck[CARD_IRONWOOD_CLEARING].linkedEnemies[0].health = 40; 
    deck[CARD_IRONWOOD_CLEARING].linkedEnemies[0].damage = 20; 
    deck[CARD_IRONWOOD_CLEARING].linkedEnemies[1].enemyType = ENEMY_GOBLIN; //goblin
    deck[CARD_IRONWOOD_CLEARING].linkedEnemies[1].health = 20; 
    deck[CARD_IRONWOOD_CLEARING].linkedEnemies[1].damage = 20;


    // Card 28  - Echoing Hollow - has event
    deck[CARD_ECHOING_HOLLOW].cardType = CARD_ECHOING_HOLLOW;
    deck[CARD_ECHOING_HOLLOW].numLinkedCards = 1;
    deck[CARD_ECHOING_HOLLOW].numLinkedItems = 2;
    strcpy(deck[CARD_ECHOING_HOLLOW].name, "Echoing Hollow");
    strcpy(deck[CARD_ECHOING_HOLLOW].description, "You are at Echoing Hollow.\nIts very windy here.\nBut if you shout, you can hear the echo...");
    deck[CARD_ECHOING_HOLLOW].linkedCards[0].cardType = CARD_MIRKWOOD_EXIT; //link back to mirkwood exit
    strcpy(deck[CARD_ECHOING_HOLLOW].linkedCards[0].name,"wood"); //link name
    deck[CARD_ECHOING_HOLLOW].linkedItems[0] = ITEM_MANA_POTION; //potion
    deck[CARD_ECHOING_HOLLOW].linkedItems[1] = ITEM_HEALTH_POTION; //potion

    // Card 29  - Cinderspire - has fireplace
    deck[CARD_CINDERSPIRE].cardType = CARD_CINDERSPIRE;
    deck[CARD_CINDERSPIRE].numLinkedCards = 1;
    strcpy(deck[CARD_CINDERSPIRE].name, "Cinderspire");
    strcpy(deck[CARD_CINDERSPIRE].description, "A jagged, smoke-streaked peak that towers over the charred landscape.\nThe air is thick with ash, and the remnants of ancient fires flicker faintly at the summit..\nThere is a fireplace built into the rock.");
    deck[CARD_CINDERSPIRE].linkedCards[0].cardType = CARD_MIRKWOOD_EXIT; //link back to mirkwood exit
    strcpy(deck[CARD_CINDERSPIRE].linkedCards[0].name,"wood"); //link name

    // Card 30  - First Gardening Shed - locked, linked to CARD_WILD_GARDEN
    deck[CARD_FIRST_SHED].cardType = CARD_FIRST_SHED;
    deck[CARD_FIRST_SHED].numLinkedCards = 1;
    deck[CARD_FIRST_SHED].numLinkedItems = 4;
    strcpy(deck[CARD_FIRST_SHED].name, "Gardening Shed");
    strcpy(deck[CARD_FIRST_SHED].description, "You are in a small gardening shed.\nThere are some tools and some soil.");
    deck[CARD_FIRST_SHED].linkedCards[0].cardType = CARD_WILD_GARDEN; //link back to wild garden
    strcpy(deck[CARD_FIRST_SHED].linkedCards[0].name,"garden"); //link name
    deck[CARD_FIRST_SHED].linkedItems[0] = ITEM_MANA_POTION; //potion
    deck[CARD_FIRST_SHED].linkedItems[1] = ITEM_HEALTH_POTION; //potion
    deck[CARD_FIRST_SHED].linkedItems[2] = ITEM_COINS; //coins
    deck[CARD_FIRST_SHED].linkedItems[3] = ITEM_COINS; //coins

    // Card 31  - Second Gardening Shed - locked, linked to CARD_IRONWOOD_CLEARING
    deck[CARD_SECOND_SHED].cardType = CARD_SECOND_SHED;
    deck[CARD_SECOND_SHED].numLinkedCards = 1;
    deck[CARD_SECOND_SHED].numLinkedItems = 4;
    strcpy(deck[CARD_SECOND_SHED].name, "Workman's Shed");
    strcpy(deck[CARD_SECOND_SHED].description, "You are in a small gardening shed.\nThere are some tools and some soil.");
    deck[CARD_SECOND_SHED].linkedCards[0].cardType = CARD_IRONWOOD_CLEARING; //link back to ironwood clearing
    strcpy(deck[CARD_SECOND_SHED].linkedCards[0].name,"clearing"); //link name
    deck[CARD_SECOND_SHED].linkedItems[0] = ITEM_MANA_POTION; //potion
    deck[CARD_SECOND_SHED].linkedItems[1] = ITEM_HEALTH_POTION; //potion
    deck[CARD_SECOND_SHED].linkedItems[2] = ITEM_COINS; //coins
    deck[CARD_SECOND_SHED].linkedItems[3] = ITEM_COINS; //coins

    // Card 32  - other side of river, unlinked until event
    deck[CARD_OTHER_SIDE_RIVER].cardType = CARD_OTHER_SIDE_RIVER;
    deck[CARD_OTHER_SIDE_RIVER].numLinkedCards = 2;
    deck[CARD_OTHER_SIDE_RIVER].numLinkedItems = 2;
    strcpy(deck[CARD_OTHER_SIDE_RIVER].name, "Other Side of the River");
    strcpy(deck[CARD_OTHER_SIDE_RIVER].description, "You cross the river to the otherside.\nThere is a trail here.");
    deck[CARD_OTHER_SIDE_RIVER].linkedCards[0].cardType = CARD_SECOND_PATH; //link back to second path
    strcpy(deck[CARD_OTHER_SIDE_RIVER].linkedCards[0].name,"tree"); //link name
    deck[CARD_OTHER_SIDE_RIVER].linkedCards[1].cardType = CARD_FIRST_TRAIL; //link to trail
    strcpy(deck[CARD_OTHER_SIDE_RIVER].linkedCards[1].name,"trail"); //link name
    deck[CARD_OTHER_SIDE_RIVER].linkedItems[0] = ITEM_COINS; //coins
    deck[CARD_OTHER_SIDE_RIVER].linkedItems[1] = ITEM_COINS; //coins

    // Card 33  - Old man McCann's - has fireplace
    deck[CARD_OLD_MAN_MCCANN].cardType = CARD_OLD_MAN_MCCANN;
    deck[CARD_OLD_MAN_MCCANN].numLinkedCards = 2;
    strcpy(deck[CARD_OLD_MAN_MCCANN].name, "Old Man McCann's");
    strcpy(deck[CARD_OLD_MAN_MCCANN].description, "Old man McCann's residence.\nThere is a path to a small garden.\nThere is a fireplace here.");
    deck[CARD_OLD_MAN_MCCANN].linkedCards[0].cardType = CARD_WRENWOOD; //link back to wrenwood
    strcpy(deck[CARD_OLD_MAN_MCCANN].linkedCards[0].name,"town"); //link name
    deck[CARD_OLD_MAN_MCCANN].linkedCards[1].cardType = CARD_MCCANN_GARDEN; //link to garden
    strcpy(deck[CARD_OLD_MAN_MCCANN].linkedCards[1].name,"garden"); //link name

    // Card 34  - Wrenwood
    deck[CARD_WRENWOOD].cardType = CARD_WRENWOOD;
    deck[CARD_WRENWOOD].numLinkedCards = 5;
    deck[CARD_WRENWOOD].numLinkedItems = 2;
    strcpy(deck[CARD_WRENWOOD].name, "Town of Wrenwood");
    strcpy(deck[CARD_WRENWOOD].description, "You are in Wrenwood.\nThere is a store here.\nThere are a few houses nearby.");
    deck[CARD_WRENWOOD].linkedCards[0].cardType = CARD_FIRST_TRAIL; //link back to trail
    strcpy(deck[CARD_WRENWOOD].linkedCards[0].name,"trail"); //link name
    deck[CARD_WRENWOOD].linkedCards[1].cardType = CARD_OLD_MAN_MCCANN; //link to McCanns
    strcpy(deck[CARD_WRENWOOD].linkedCards[1].name,"mccann"); //link name
    deck[CARD_WRENWOOD].linkedCards[2].cardType = CARD_SECOND_STORE; //link to store
    strcpy(deck[CARD_WRENWOOD].linkedCards[2].name,"store"); //link name
    deck[CARD_WRENWOOD].linkedCards[3].cardType = CARD_GREY; //link to alistair grey's
    strcpy(deck[CARD_WRENWOOD].linkedCards[3].name,"grey"); //link name
    deck[CARD_WRENWOOD].linkedCards[4].cardType = CARD_WRENWOOD_ROAD; //link east to road
    strcpy(deck[CARD_WRENWOOD].linkedCards[4].name,"east"); //link name
    deck[CARD_WRENWOOD].linkedItems[0] = ITEM_COINS; //coins
    deck[CARD_WRENWOOD].linkedItems[1] = ITEM_COINS; //coins

    // Card 35  - First Trail
    deck[CARD_FIRST_TRAIL].cardType = CARD_FIRST_TRAIL;
    deck[CARD_FIRST_TRAIL].numLinkedCards = 2;
    strcpy(deck[CARD_FIRST_TRAIL].name, "Trail in the woods");
    strcpy(deck[CARD_FIRST_TRAIL].description, "You follow the trail into the woods.\nYou are near Wrenwood.");
    deck[CARD_FIRST_TRAIL].linkedCards[0].cardType = CARD_OTHER_SIDE_RIVER; //link back to mirkwood exit
    strcpy(deck[CARD_FIRST_TRAIL].linkedCards[0].name,"river"); //link name
    deck[CARD_FIRST_TRAIL].linkedCards[1].cardType = CARD_WRENWOOD; //link back to mirkwood exit
    strcpy(deck[CARD_FIRST_TRAIL].linkedCards[1].name,"town"); //link name

    // Card 36  - General store / second store
    deck[CARD_SECOND_STORE].cardType = CARD_SECOND_STORE;
    deck[CARD_SECOND_STORE].isStore = true;
    deck[CARD_SECOND_STORE].numLinkedCards = 1;
    strcpy(deck[CARD_SECOND_STORE].name, "General Store");
    strcpy(deck[CARD_SECOND_STORE].description, "A small store where you can buy potions.");
    deck[CARD_SECOND_STORE].linkedCards[0].cardType = CARD_WRENWOOD; //link back to wrenwood
    strcpy(deck[CARD_SECOND_STORE].linkedCards[0].name,"town"); //link name

    // Card 37  - McCann Garden
    deck[CARD_MCCANN_GARDEN].cardType = CARD_MCCANN_GARDEN;
    deck[CARD_MCCANN_GARDEN].numLinkedCards = 1;
    strcpy(deck[CARD_MCCANN_GARDEN].name, "Old man McCann's Garden");
    strcpy(deck[CARD_MCCANN_GARDEN].description, "You are in old man McCann's nice garden.\nThere are lots of plants to choose from.\nThere is a tomatoe plant.\nThere is also a shed here but the door is locked.");
    deck[CARD_MCCANN_GARDEN].linkedCards[0].cardType = CARD_OLD_MAN_MCCANN; //link back to mccanns
    strcpy(deck[CARD_MCCANN_GARDEN].linkedCards[0].name,"house"); //link name

    // Card 38  - Third Gardening Shed - locked, linked to CARD_MCCANN_GARDEN
    deck[CARD_THIRD_SHED].cardType = CARD_THIRD_SHED;
    deck[CARD_THIRD_SHED].numLinkedCards = 1;
    deck[CARD_THIRD_SHED].numLinkedItems = 4;
    strcpy(deck[CARD_THIRD_SHED].name, "McCann's Shed");
    strcpy(deck[CARD_THIRD_SHED].description, "You are in a small gardening shed.\nThere are some tools and some soil.");
    deck[CARD_THIRD_SHED].linkedCards[0].cardType = CARD_MCCANN_GARDEN; //link back to mccann garden
    strcpy(deck[CARD_THIRD_SHED].linkedCards[0].name,"garden"); //link name
    deck[CARD_THIRD_SHED].linkedItems[0] = ITEM_MANA_POTION; //potion
    deck[CARD_THIRD_SHED].linkedItems[1] = ITEM_MANA_POTION; //potion
    deck[CARD_THIRD_SHED].linkedItems[2] = ITEM_COINS; //coins
    deck[CARD_THIRD_SHED].linkedItems[3] = ITEM_COINS; //coins

    // Card 39  - Lost in the woods sequence 1
    deck[CARD_LOST_WOODS1].cardType = CARD_LOST_WOODS1;
    deck[CARD_LOST_WOODS1].numLinkedCards = 1;
    deck[CARD_LOST_WOODS1].numLinkedItems = 2;
    strcpy(deck[CARD_LOST_WOODS1].name, "Deep in the Woods");
    strcpy(deck[CARD_LOST_WOODS1].description, "You are Lost in the woods.");
    deck[CARD_LOST_WOODS1].linkedCards[0].cardType = CARD_LOST_WOODS2; //link to lost in the woods 2
    strcpy(deck[CARD_LOST_WOODS1].linkedCards[0].name,"lost"); //link name
    deck[CARD_LOST_WOODS1].linkedItems[0] = ITEM_MANA_POTION; //potion
    deck[CARD_LOST_WOODS1].linkedItems[1] = ITEM_MANA_POTION; //potion

    // Card 40  - Lost in the woods sequence 2
    deck[CARD_LOST_WOODS2].cardType = CARD_LOST_WOODS2;
    deck[CARD_LOST_WOODS2].numLinkedCards = 1;
    deck[CARD_LOST_WOODS2].numLinkedItems = 2;
    strcpy(deck[CARD_LOST_WOODS2].name, "Lost in the Woods");
    strcpy(deck[CARD_LOST_WOODS2].description, "You are Lost in the woods.");
    deck[CARD_LOST_WOODS2].linkedCards[0].cardType = CARD_LOST_WOODS3; //link to lost in the woods 3
    strcpy(deck[CARD_LOST_WOODS2].linkedCards[0].name,"lost"); //link name
    deck[CARD_LOST_WOODS2].linkedItems[0] = ITEM_MANA_POTION; //potion
    deck[CARD_LOST_WOODS2].linkedItems[1] = ITEM_MANA_POTION; //potion

    // Card 41  - Lost in the woods sequence 3
    deck[CARD_LOST_WOODS3].cardType = CARD_LOST_WOODS3;
    deck[CARD_LOST_WOODS3].numLinkedCards = 1;
    deck[CARD_LOST_WOODS3].numLinkedItems = 2;
    strcpy(deck[CARD_LOST_WOODS3].name, "Lost in the Deep Woods");
    strcpy(deck[CARD_LOST_WOODS3].description, "You are Lost deep in the woods.\nAs you walk further, suddenly the brush subsides.\nThere seems to be a small garden up ahead.");
    deck[CARD_LOST_WOODS3].linkedCards[0].cardType = CARD_JEFFERS_GARDEN; //link to Jeffers Garden
    strcpy(deck[CARD_LOST_WOODS3].linkedCards[0].name,"garden"); //link name
    deck[CARD_LOST_WOODS3].linkedItems[0] = ITEM_MANA_POTION; //potion
    deck[CARD_LOST_WOODS3].linkedItems[1] = ITEM_MANA_POTION; //potion

    // Card 42  - First Bridge
    deck[CARD_FIRST_BRIDGE].cardType = CARD_FIRST_BRIDGE;
    deck[CARD_FIRST_BRIDGE].numLinkedCards = 2;
    deck[CARD_FIRST_BRIDGE].numLinkedEnemies = 1;
    strcpy(deck[CARD_FIRST_BRIDGE].name, "Bridge near Bramblethorn Overlook");
    strcpy(deck[CARD_FIRST_BRIDGE].description, "You make your way to the bridge.\nA scenic crossing point over serene waters,\nwhere the wooden slats creak with the\nwhispers of secrets shared between friends.");
    deck[CARD_FIRST_BRIDGE].linkedCards[0].cardType = CARD_BRAMBLETHORN_OVERLOOK; //link back to overlook
    strcpy(deck[CARD_FIRST_BRIDGE].linkedCards[0].name,"overlook"); //link name
    deck[CARD_FIRST_BRIDGE].linkedCards[1].cardType = CARD_WILLIAMS; //link to williams
    strcpy(deck[CARD_FIRST_BRIDGE].linkedCards[1].name,"town"); //link name
    deck[CARD_FIRST_BRIDGE].linkedEnemies[0].enemyType = ENEMY_GOBLIN; //goblin
    deck[CARD_FIRST_BRIDGE].linkedEnemies[0].health = 100; 
    deck[CARD_FIRST_BRIDGE].linkedEnemies[0].damage = 50;

    // Card 43  - Williams
    deck[CARD_WILLIAMS].cardType = CARD_WILLIAMS;
    deck[CARD_WILLIAMS].numLinkedCards = 3;
    deck[CARD_WILLIAMS].numLinkedItems = 4;
    strcpy(deck[CARD_WILLIAMS].name, "Town of Williams");
    strcpy(deck[CARD_WILLIAMS].description, "You are in the town of Williams.\nIts very nice here.\nThere is a small store here.\nTown hall is nearby but the door is locked.\nA small road leads north out of town.");
    deck[CARD_WILLIAMS].linkedCards[0].cardType = CARD_FIRST_BRIDGE; //link back to bridge
    strcpy(deck[CARD_WILLIAMS].linkedCards[0].name,"bridge"); //link name
    deck[CARD_WILLIAMS].linkedCards[1].cardType = CARD_WILLIAMS_STORE; //link to store
    strcpy(deck[CARD_WILLIAMS].linkedCards[1].name,"store"); //link name
    deck[CARD_WILLIAMS].linkedCards[2].cardType = CARD_WILLIAMS_ROAD; //link to road
    strcpy(deck[CARD_WILLIAMS].linkedCards[2].name,"road"); //link name
    deck[CARD_WILLIAMS].linkedItems[0] = ITEM_MANA_POTION; //potion
    deck[CARD_WILLIAMS].linkedItems[1] = ITEM_HEALTH_POTION; //potion
    deck[CARD_WILLIAMS].linkedItems[2] = ITEM_COINS; //coins
    deck[CARD_WILLIAMS].linkedItems[3] = ITEM_COINS; //coins

    // Card 44  - Williams General store
    deck[CARD_WILLIAMS_STORE].cardType = CARD_WILLIAMS_STORE;
    deck[CARD_WILLIAMS_STORE].isStore = true;
    deck[CARD_WILLIAMS_STORE].numLinkedCards = 1;
    strcpy(deck[CARD_WILLIAMS_STORE].name, "Williams General Store");
    strcpy(deck[CARD_WILLIAMS_STORE].description, "You are in a small store.\nBuy items.");
    deck[CARD_WILLIAMS_STORE].linkedCards[0].cardType = CARD_WILLIAMS; //link back to williams
    strcpy(deck[CARD_WILLIAMS_STORE].linkedCards[0].name,"town"); //link name

    // Card 45  - Williams Town Hall - locked, linked to CARD_WILLIAMS, also has fireplace
    deck[CARD_WILLIAMS_TOWN_HALL].cardType = CARD_WILLIAMS_TOWN_HALL;
    deck[CARD_WILLIAMS_TOWN_HALL].numLinkedCards = 1;
    strcpy(deck[CARD_WILLIAMS_TOWN_HALL].name, "Williams Town Hall");
    strcpy(deck[CARD_WILLIAMS_TOWN_HALL].description, "You are in Williams Town Hall.\nThere is a fire place here.");
    deck[CARD_WILLIAMS_TOWN_HALL].linkedCards[0].cardType = CARD_WILLIAMS; //link back to williams
    strcpy(deck[CARD_WILLIAMS_TOWN_HALL].linkedCards[0].name,"town"); //link name

    // Card 46  - Road north of Williams
    deck[CARD_WILLIAMS_ROAD].cardType = CARD_WILLIAMS_ROAD;
    deck[CARD_WILLIAMS_ROAD].numLinkedCards = 4;
    deck[CARD_WILLIAMS_ROAD].numLinkedEnemies = 5;
    strcpy(deck[CARD_WILLIAMS_ROAD].name, "Road north of Williams");
    strcpy(deck[CARD_WILLIAMS_ROAD].description, "You walk along the road north of Willams until you reach a split.\nYou can go north, east, west, or south back to Williams.");
    deck[CARD_WILLIAMS_ROAD].linkedCards[0].cardType = CARD_WILLIAMS; //link back to williams
    strcpy(deck[CARD_WILLIAMS_ROAD].linkedCards[0].name,"south"); //link name
    deck[CARD_WILLIAMS_ROAD].linkedCards[1].cardType = CARD_STACK_HOMESTEAD; //link back to williams
    strcpy(deck[CARD_WILLIAMS_ROAD].linkedCards[1].name,"west"); //link name
    deck[CARD_WILLIAMS_ROAD].linkedCards[2].cardType = CARD_ELDERFERN_FOREST_ENTER; //link back to williams
    strcpy(deck[CARD_WILLIAMS_ROAD].linkedCards[2].name,"east"); //link name
    deck[CARD_WILLIAMS_ROAD].linkedCards[3].cardType = CARD_EMBERFELL_CASTLE_ENTER; //link back to williams
    strcpy(deck[CARD_WILLIAMS_ROAD].linkedCards[3].name,"north"); //link name
    deck[CARD_WILLIAMS_ROAD].linkedEnemies[0].enemyType = ENEMY_ORC; //orc
    deck[CARD_WILLIAMS_ROAD].linkedEnemies[0].health = 100; 
    deck[CARD_WILLIAMS_ROAD].linkedEnemies[0].damage = 100;
    deck[CARD_WILLIAMS_ROAD].linkedEnemies[1].enemyType = ENEMY_OGRE; //ogre
    deck[CARD_WILLIAMS_ROAD].linkedEnemies[1].health = 100; 
    deck[CARD_WILLIAMS_ROAD].linkedEnemies[1].damage = 80;
    deck[CARD_WILLIAMS_ROAD].linkedEnemies[2].enemyType = ENEMY_GOBLIN; //goblin
    deck[CARD_WILLIAMS_ROAD].linkedEnemies[2].health = 20; 
    deck[CARD_WILLIAMS_ROAD].linkedEnemies[2].damage = 10;
    deck[CARD_WILLIAMS_ROAD].linkedEnemies[3].enemyType = ENEMY_GOBLIN; //goblin
    deck[CARD_WILLIAMS_ROAD].linkedEnemies[3].health = 30; 
    deck[CARD_WILLIAMS_ROAD].linkedEnemies[3].damage = 20;
    deck[CARD_WILLIAMS_ROAD].linkedEnemies[4].enemyType = ENEMY_GOBLIN; //goblin
    deck[CARD_WILLIAMS_ROAD].linkedEnemies[4].health = 40; 
    deck[CARD_WILLIAMS_ROAD].linkedEnemies[4].damage = 30;

    // Card 47  - Stack Homestead - has fireplace
    deck[CARD_STACK_HOMESTEAD].cardType = CARD_STACK_HOMESTEAD;
    deck[CARD_STACK_HOMESTEAD].numLinkedCards = 1;
    strcpy(deck[CARD_STACK_HOMESTEAD].name, "Stack Homestead");
    strcpy(deck[CARD_STACK_HOMESTEAD].description, "You are at the Stack Homestead.\nThere is a mountain to the north.\nOn the farside of the homestead, there is a river.\nThe river is too deep to cross.\nThere is a pumpkin patch.\nThere is a fireplace here.");
    deck[CARD_STACK_HOMESTEAD].linkedCards[0].cardType = CARD_WILLIAMS_ROAD; //link back to williams road
    strcpy(deck[CARD_STACK_HOMESTEAD].linkedCards[0].name,"east"); //link name
    deck[CARD_STACK_HOMESTEAD].linkedCards[1].cardType = CARD_MOUNTAIN; //link to mountain
    strcpy(deck[CARD_STACK_HOMESTEAD].linkedCards[1].name,"path"); //link name
    
    // Card 48  - Elderfern Forest Entrance
    deck[CARD_ELDERFERN_FOREST_ENTER].cardType = CARD_ELDERFERN_FOREST_ENTER;
    deck[CARD_ELDERFERN_FOREST_ENTER].numLinkedCards = 2;
    strcpy(deck[CARD_ELDERFERN_FOREST_ENTER].name, "Elderfern Forest Entrance");
    strcpy(deck[CARD_ELDERFERN_FOREST_ENTER].description, "You walk until you reach the entrance to Elderfern Forest.\nThe winding paths through Elderfern are often shrouded in mist,\nmaking it easy to lose one's way and harder still to shake\nthe feeling of being watched by unseen eyes.");
    deck[CARD_ELDERFERN_FOREST_ENTER].linkedCards[0].cardType = CARD_WILLIAMS_ROAD; //link back to williams road
    strcpy(deck[CARD_ELDERFERN_FOREST_ENTER].linkedCards[0].name,"west"); //link name
    deck[CARD_ELDERFERN_FOREST_ENTER].linkedCards[1].cardType = CARD_ELDERFERN_1; //link to wood loop
    strcpy(deck[CARD_ELDERFERN_FOREST_ENTER].linkedCards[1].name,"wood"); //link name
    
    // Card 49  - Emberfell Castle Entrance
    deck[CARD_EMBERFELL_CASTLE_ENTER].cardType = CARD_EMBERFELL_CASTLE_ENTER;
    deck[CARD_EMBERFELL_CASTLE_ENTER].numLinkedCards = 1;
    strcpy(deck[CARD_EMBERFELL_CASTLE_ENTER].name, "Emberfell Castle Entrance");
    strcpy(deck[CARD_EMBERFELL_CASTLE_ENTER].description, "You are outside of Emberfell castle.\nIts blackened stone walls rising dramatically against the backdrop of a stormy sky.\nOnce a stronghold of ancient kings,\nit is now a haunting silhouette in the mist.\nThe door is locked.");
    deck[CARD_EMBERFELL_CASTLE_ENTER].linkedCards[0].cardType = CARD_WILLIAMS_ROAD; //link back to williams road
    strcpy(deck[CARD_EMBERFELL_CASTLE_ENTER].linkedCards[0].name,"south"); //link name

    // Card 50  - Elderfern Forest 1
    deck[CARD_ELDERFERN_1].cardType = CARD_ELDERFERN_1;
    deck[CARD_ELDERFERN_1].numLinkedCards = 1;
    deck[CARD_ELDERFERN_1].numLinkedEnemies = 2;
    strcpy(deck[CARD_ELDERFERN_1].name, "Elderfern Forest Clearing");
    strcpy(deck[CARD_ELDERFERN_1].description, "You are lost in the forest.");
    deck[CARD_ELDERFERN_1].linkedCards[0].cardType = CARD_ELDERFERN_2; //link back to forest next
    strcpy(deck[CARD_ELDERFERN_1].linkedCards[0].name,"wood"); //link name
    deck[CARD_ELDERFERN_1].linkedEnemies[0].enemyType = ENEMY_WOLF; //wolf
    deck[CARD_ELDERFERN_1].linkedEnemies[0].health = 100; 
    deck[CARD_ELDERFERN_1].linkedEnemies[0].damage = 100;
    deck[CARD_ELDERFERN_1].linkedEnemies[1].enemyType = ENEMY_WOLF; //wolf
    deck[CARD_ELDERFERN_1].linkedEnemies[1].health = 100; 
    deck[CARD_ELDERFERN_1].linkedEnemies[1].damage = 80;

    // Card 51  - Elderfern Forest 2
    deck[CARD_ELDERFERN_2].cardType = CARD_ELDERFERN_2;
    deck[CARD_ELDERFERN_2].numLinkedCards = 1;
    strcpy(deck[CARD_ELDERFERN_2].name, "Elderfern Forest Path");
    strcpy(deck[CARD_ELDERFERN_2].description, "You are lost in the forest.");
    deck[CARD_ELDERFERN_2].linkedCards[0].cardType = CARD_ELDERFERN_3; //link back to forest next
    strcpy(deck[CARD_ELDERFERN_2].linkedCards[0].name,"wood"); //link name

    // Card 52  - Elderfern Forest 3
    deck[CARD_ELDERFERN_3].cardType = CARD_ELDERFERN_3;
    deck[CARD_ELDERFERN_3].numLinkedCards = 1;
    deck[CARD_ELDERFERN_3].numLinkedEnemies = 2;
    strcpy(deck[CARD_ELDERFERN_3].name, "Elderfern Forest Brush");
    strcpy(deck[CARD_ELDERFERN_3].description, "You are lost in the forest.");
    deck[CARD_ELDERFERN_3].linkedCards[0].cardType = CARD_ELDERFERN_4; //link back to forest next
    strcpy(deck[CARD_ELDERFERN_3].linkedCards[0].name,"wood"); //link name
    deck[CARD_ELDERFERN_3].linkedEnemies[0].enemyType = ENEMY_WOLF; //wolf
    deck[CARD_ELDERFERN_3].linkedEnemies[0].health = 100; 
    deck[CARD_ELDERFERN_3].linkedEnemies[0].damage = 100;
    deck[CARD_ELDERFERN_3].linkedEnemies[1].enemyType = ENEMY_WOLF; //wolf
    deck[CARD_ELDERFERN_3].linkedEnemies[1].health = 100; 
    deck[CARD_ELDERFERN_3].linkedEnemies[1].damage = 80;

    // Card 53  - Elderfern Forest 4
    deck[CARD_ELDERFERN_4].cardType = CARD_ELDERFERN_4;
    deck[CARD_ELDERFERN_4].numLinkedCards = 1;
    deck[CARD_ELDERFERN_4].numLinkedItems = 5;
    strcpy(deck[CARD_ELDERFERN_4].name, "Elderfern Forest");
    strcpy(deck[CARD_ELDERFERN_4].description, "You are lost in the forest.");
    deck[CARD_ELDERFERN_4].linkedCards[0].cardType = CARD_ELDERFERN_FOREST_ENTER; //link back to forest out
    strcpy(deck[CARD_ELDERFERN_4].linkedCards[0].name,"wood"); //link name
    deck[CARD_ELDERFERN_4].linkedItems[0] = ITEM_MANA_POTION; //potion
    deck[CARD_ELDERFERN_4].linkedItems[1] = ITEM_MANA_POTION; //potion
    deck[CARD_ELDERFERN_4].linkedItems[2] = ITEM_MANA_POTION; //potion
    deck[CARD_ELDERFERN_4].linkedItems[3] = ITEM_MANA_POTION; //potion
    deck[CARD_ELDERFERN_4].linkedItems[4] = ITEM_MANA_POTION; //potion

    // Card 54  - The Emberhall
    deck[CARD_EMBERFELL_HALL].cardType = CARD_EMBERFELL_HALL;
    deck[CARD_EMBERFELL_HALL].numLinkedCards = 4;
    deck[CARD_EMBERFELL_HALL].numLinkedItems = 1;
    strcpy(deck[CARD_EMBERFELL_HALL].name, "The Emberhall");
    strcpy(deck[CARD_EMBERFELL_HALL].description, "You are in the grand entrance hall,\nwhere flickering torches cast long shadows on the stone walls.\nThe echoes of footsteps seem to linger long after someone has passed through.");
    deck[CARD_EMBERFELL_HALL].linkedCards[0].cardType = CARD_EMBERFELL_CASTLE_ENTER; //link back to entrance
    strcpy(deck[CARD_EMBERFELL_HALL].linkedCards[0].name,"out"); //link name
    deck[CARD_EMBERFELL_HALL].linkedCards[1].cardType = CARD_EMBERFELL_CHAMBER; //link to chamber
    strcpy(deck[CARD_EMBERFELL_HALL].linkedCards[1].name,"chamber"); //link name
    deck[CARD_EMBERFELL_HALL].linkedCards[2].cardType = CARD_EMBERFELL_LIBRARY; //link to library
    strcpy(deck[CARD_EMBERFELL_HALL].linkedCards[2].name,"books"); //link name
    deck[CARD_EMBERFELL_HALL].linkedCards[3].cardType = CARD_EMBERFELL_STAIRS; //link stairs
    strcpy(deck[CARD_EMBERFELL_HALL].linkedCards[3].name,"stairs"); //link name
    deck[CARD_EMBERFELL_HALL].linkedItems[0] = ITEM_MANA_POTION; //potion

    // Card 55  - The Chamber of Ashes
    deck[CARD_EMBERFELL_CHAMBER].cardType = CARD_EMBERFELL_CHAMBER;
    deck[CARD_EMBERFELL_CHAMBER].numLinkedCards = 1;
    strcpy(deck[CARD_EMBERFELL_CHAMBER].name, "The Chamber of Ashes");
    strcpy(deck[CARD_EMBERFELL_CHAMBER].description, "A small, private room where the remains of past monarchs are kept in urns.\nA haunting silence fills the air,\nbroken only by the occasional gust of wind through the cracks in the walls.\nThe ashes are said to hold the spirits of the fallen,\ntheir whispers faint but ever present.");
    deck[CARD_EMBERFELL_CHAMBER].linkedCards[0].cardType = CARD_EMBERFELL_HALL; //link back to hall
    strcpy(deck[CARD_EMBERFELL_CHAMBER].linkedCards[0].name,"hall"); //link name

    // Card 56  - The Library of Forgotten Lore
    deck[CARD_EMBERFELL_LIBRARY].cardType = CARD_EMBERFELL_LIBRARY;
    deck[CARD_EMBERFELL_LIBRARY].numLinkedCards = 2;
    deck[CARD_EMBERFELL_LIBRARY].numLinkedItems = 1;
    strcpy(deck[CARD_EMBERFELL_LIBRARY].name, "The Library of Forgotten Lore");
    strcpy(deck[CARD_EMBERFELL_LIBRARY].description, "An immense, dusty library,\nfilled with ancient tomes, scrolls, and forbidden texts.\nThe bookshelves are stacked high with knowledge lost to time,\nand some say the books themselves can whisper dark secrets if one listens closely enough.");
    deck[CARD_EMBERFELL_LIBRARY].linkedCards[0].cardType = CARD_EMBERFELL_HALL; //link back to hall
    strcpy(deck[CARD_EMBERFELL_LIBRARY].linkedCards[0].name,"hall"); //link name
    deck[CARD_EMBERFELL_LIBRARY].linkedCards[1].cardType = CARD_EMBERFELL_GARDEN; //link to garden
    strcpy(deck[CARD_EMBERFELL_LIBRARY].linkedCards[1].name,"garden"); //link name
    deck[CARD_EMBERFELL_LIBRARY].linkedItems[0] = ITEM_BOOK; //book

    // Card 57  - The Forgotten Garden
    deck[CARD_EMBERFELL_GARDEN].cardType = CARD_EMBERFELL_GARDEN;
    deck[CARD_EMBERFELL_GARDEN].numLinkedCards = 1;
    strcpy(deck[CARD_EMBERFELL_GARDEN].name, "The Forgotten Garden");
    strcpy(deck[CARD_EMBERFELL_GARDEN].description, "A once beautiful garden now overrun with tangled vines,\ntwisted trees,\nand flowers that bloom in strange, otherworldly colors.\nThere is a tomatoe plant here.");
    deck[CARD_EMBERFELL_GARDEN].linkedCards[0].cardType = CARD_EMBERFELL_LIBRARY; //link back to library
    strcpy(deck[CARD_EMBERFELL_GARDEN].linkedCards[0].name,"books"); //link name

    // Card 58  - Spiral Staircase
    deck[CARD_EMBERFELL_STAIRS].cardType = CARD_EMBERFELL_STAIRS;
    deck[CARD_EMBERFELL_STAIRS].numLinkedCards = 3;
    strcpy(deck[CARD_EMBERFELL_STAIRS].name, "Spiral Staircase");
    strcpy(deck[CARD_EMBERFELL_STAIRS].description, "You are on a large spiral staricase. You can go up or down.");
    deck[CARD_EMBERFELL_STAIRS].linkedCards[0].cardType = CARD_EMBERFELL_HALL; //link back to hall
    strcpy(deck[CARD_EMBERFELL_STAIRS].linkedCards[0].name,"hall"); //link name
    deck[CARD_EMBERFELL_STAIRS].linkedCards[1].cardType = CARD_EMBERFELL_TOWER; //link to tower
    strcpy(deck[CARD_EMBERFELL_STAIRS].linkedCards[1].name,"up"); //link name
    deck[CARD_EMBERFELL_STAIRS].linkedCards[2].cardType = CARD_EMBERFELL_DUNGEON; //link to dungeon
    strcpy(deck[CARD_EMBERFELL_STAIRS].linkedCards[2].name,"down"); //link name

    // Card 59  - The Dungeon of Emberfell
    deck[CARD_EMBERFELL_DUNGEON].cardType = CARD_EMBERFELL_DUNGEON;
    deck[CARD_EMBERFELL_DUNGEON].numLinkedCards = 2;
    deck[CARD_EMBERFELL_DUNGEON].numLinkedEnemies = 1;
    strcpy(deck[CARD_EMBERFELL_DUNGEON].name, "The Dungeon of Emberfell");
    strcpy(deck[CARD_EMBERFELL_DUNGEON].description, "You take the staircase down, beneath the castle.\nShackles and chains still hang from the walls,\nand the faint sound of water dripping echoes throughout.\nIt is said that those imprisoned here were never meant to leave.");
    deck[CARD_EMBERFELL_DUNGEON].linkedCards[0].cardType = CARD_EMBERFELL_STAIRS; //link back to stairs
    strcpy(deck[CARD_EMBERFELL_DUNGEON].linkedCards[0].name,"up"); //link name
    deck[CARD_EMBERFELL_DUNGEON].linkedCards[1].cardType = CARD_EMBERFELL_ROAD; //link out to road
    strcpy(deck[CARD_EMBERFELL_DUNGEON].linkedCards[1].name,"out"); //link name
    deck[CARD_EMBERFELL_DUNGEON].linkedEnemies[0].enemyType = ENEMY_ORC; //orc
    deck[CARD_EMBERFELL_DUNGEON].linkedEnemies[0].health = 400; 
    deck[CARD_EMBERFELL_DUNGEON].linkedEnemies[0].damage = 100;

    // Card 60  - The Ashen Tower
    deck[CARD_EMBERFELL_TOWER].cardType = CARD_EMBERFELL_TOWER;
    deck[CARD_EMBERFELL_TOWER].numLinkedCards = 1;
    strcpy(deck[CARD_EMBERFELL_TOWER].name, "The Ashen Tower");
    strcpy(deck[CARD_EMBERFELL_TOWER].description, "You climb the spiral staircase all the way to the top of the castle.\nThere is a small room at the top.\nThe room contains a large treasure chest, but it is locked.");
    deck[CARD_EMBERFELL_TOWER].linkedCards[0].cardType = CARD_EMBERFELL_STAIRS; //link back to spiral staircase
    strcpy(deck[CARD_EMBERFELL_TOWER].linkedCards[0].name,"down"); //link name

    // Card 61  - Road outside Emberfell Dungeon
    deck[CARD_EMBERFELL_ROAD].cardType = CARD_EMBERFELL_ROAD;
    deck[CARD_EMBERFELL_ROAD].numLinkedCards = 4;
    strcpy(deck[CARD_EMBERFELL_ROAD].name, "Road outside Emberfell Dungeon");
    strcpy(deck[CARD_EMBERFELL_ROAD].description, "You are just outside Emberfell Castle.\nYou can enter through the dungeon.\nYou can also go north, east or west.");
    deck[CARD_EMBERFELL_ROAD].linkedCards[0].cardType = CARD_EMBERFELL_DUNGEON; //link back to dungeon
    strcpy(deck[CARD_EMBERFELL_ROAD].linkedCards[0].name,"castle"); //link name
    deck[CARD_EMBERFELL_ROAD].linkedCards[1].cardType = CARD_VILLAGE; //
    strcpy(deck[CARD_EMBERFELL_ROAD].linkedCards[1].name,"north"); //link name
    deck[CARD_EMBERFELL_ROAD].linkedCards[2].cardType = CARD_TOMBS; //
    strcpy(deck[CARD_EMBERFELL_ROAD].linkedCards[2].name,"west"); //link name
    deck[CARD_EMBERFELL_ROAD].linkedCards[3].cardType = CARD_CAVERNS; //
    strcpy(deck[CARD_EMBERFELL_ROAD].linkedCards[3].name,"east"); //link name

    // Card 62  - Alistair Grey's House
    deck[CARD_GREY].cardType = CARD_GREY;
    deck[CARD_GREY].numLinkedCards = 1;
    strcpy(deck[CARD_GREY].name, "Alistair Grey's House");
    strcpy(deck[CARD_GREY].description, "You are at Alistair Grey's House.\nAlistair is here.\nHe looks at you and says 'I am looking for some good books,\nIf you find any and bring them to me,\nI will make it worth your while'.");
    deck[CARD_GREY].linkedCards[0].cardType = CARD_WRENWOOD; //link back to wrenwood
    strcpy(deck[CARD_GREY].linkedCards[0].name,"town"); //link name

    // Card 63  - Across the river from the Stack Homestead
    deck[CARD_STACK_RIVER].cardType = CARD_STACK_RIVER;
    deck[CARD_STACK_RIVER].numLinkedCards = 2;
    strcpy(deck[CARD_STACK_RIVER].name, "Across the river from the Stack Homestead");
    strcpy(deck[CARD_STACK_RIVER].description, "You are on the other side of the river from the Stack Homestead.\nThere is a path...");
    deck[CARD_STACK_RIVER].linkedCards[0].cardType = CARD_STACK_HOMESTEAD; //link back to stack homestead
    strcpy(deck[CARD_STACK_RIVER].linkedCards[0].name,"tree"); //link name
    deck[CARD_STACK_RIVER].linkedCards[1].cardType = CARD_OBSERVATORY; //link back to stack homestead
    strcpy(deck[CARD_STACK_RIVER].linkedCards[1].name,"path"); //link name

    // Card 64  - The Celestial Observatory
    deck[CARD_OBSERVATORY].cardType = CARD_OBSERVATORY;
    deck[CARD_OBSERVATORY].numLinkedCards = 1;
    deck[CARD_OBSERVATORY].numLinkedItems = 1;
    strcpy(deck[CARD_OBSERVATORY].name, "The Celestial Observatory");
    strcpy(deck[CARD_OBSERVATORY].description, "You take the path to the Observatory.\nIts stone walls, adorned with celestial carvings,\nare weathered but sturdy, exuding an air of forgotten knowledge.\nInside, a grand telescope dominates the room,\nits crystal lenses glowing faintly as it seems to reach for the stars.\nThe observatory is filled with dusty tomes, celestial maps, and mysterious artifacts,\nall hinting at arcane studies of the heavens.");
    deck[CARD_OBSERVATORY].linkedCards[0].cardType = CARD_STACK_RIVER; //link back to river next to stack homestead
    strcpy(deck[CARD_OBSERVATORY].linkedCards[0].name,"path"); //link name
    deck[CARD_OBSERVATORY].linkedItems[0] = ITEM_BOOK; //book

    // Card 65  - Crystal Caverns
    deck[CARD_CAVERNS].cardType = CARD_CAVERNS;
    deck[CARD_CAVERNS].numLinkedCards = 2;
    strcpy(deck[CARD_CAVERNS].name, "Crystal Caverns");
    strcpy(deck[CARD_CAVERNS].description, "A breathtaking underground realm of glittering beauty and eerie silence.\nThe walls of the cavern are lined with towering,\ntranslucent crystals that glow with an ethereal light,\ncasting prismatic reflections across the dark, winding tunnels.\nThere is a door here but it is locked.");
    deck[CARD_CAVERNS].linkedCards[0].cardType = CARD_GLYPHS; //?
    strcpy(deck[CARD_CAVERNS].linkedCards[0].name,"north"); //link name
    deck[CARD_CAVERNS].linkedCards[1].cardType = CARD_EMBERFELL_ROAD; //?
    strcpy(deck[CARD_CAVERNS].linkedCards[1].name,"west"); //link name

    // Card 66  - Ancient Tombs
    deck[CARD_TOMBS].cardType = CARD_TOMBS;
    deck[CARD_TOMBS].numLinkedCards = 2;
    strcpy(deck[CARD_TOMBS].name, "Ancient Tombs");
    strcpy(deck[CARD_TOMBS].description, "A dark, forgotten burial ground carved into the heart of a desolate hillside.\nThe air is heavy with the scent of dust and decay as stone doorways lead into shadowed chambers,\nwhere the remains of long-dead rulers and warriors lie undisturbed.\nFaded carvings and crumbling inscriptions line the walls,\ntelling tales of a lost civilization.");
    deck[CARD_TOMBS].linkedCards[0].cardType = CARD_PEAKS; //?
    strcpy(deck[CARD_TOMBS].linkedCards[0].name,"north"); //link name
    deck[CARD_TOMBS].linkedCards[1].cardType = CARD_EMBERFELL_ROAD; //?
    strcpy(deck[CARD_TOMBS].linkedCards[1].name,"east"); //link name

    // Card 67  - The Enchanted Village - has fireplace
    deck[CARD_VILLAGE].cardType = CARD_VILLAGE;
    deck[CARD_VILLAGE].numLinkedCards = 5;
    deck[CARD_VILLAGE].numLinkedItems = 3;
    strcpy(deck[CARD_VILLAGE].name, "The Enchanted Village");
    strcpy(deck[CARD_VILLAGE].description, "A quaint, seemingly timeless settlement hidden deep within the forest.\nIts cobblestone streets are lined with charming cottages,\ntheir windows glowing warmly with light.\nThere is a fire place here.");
    deck[CARD_VILLAGE].linkedCards[0].cardType = CARD_FORSAKEN_ROAD; //?
    strcpy(deck[CARD_VILLAGE].linkedCards[0].name,"north"); //link name
    deck[CARD_VILLAGE].linkedCards[1].cardType = CARD_EMBERFELL_ROAD; //?
    strcpy(deck[CARD_VILLAGE].linkedCards[1].name,"south"); //link name
    deck[CARD_VILLAGE].linkedCards[2].cardType = CARD_GLYPHS; //?
    strcpy(deck[CARD_VILLAGE].linkedCards[2].name,"east"); //link name
    deck[CARD_VILLAGE].linkedCards[3].cardType = CARD_PEAKS; //?
    strcpy(deck[CARD_VILLAGE].linkedCards[3].name,"west"); //link name
    deck[CARD_VILLAGE].linkedCards[4].cardType = CARD_VILLAGE_STORE; //?
    strcpy(deck[CARD_VILLAGE].linkedCards[4].name,"store"); //link name
    deck[CARD_VILLAGE].linkedItems[0] = ITEM_MANA_POTION; //potion
    deck[CARD_VILLAGE].linkedItems[1] = ITEM_HEALTH_POTION; //potion
    deck[CARD_VILLAGE].linkedItems[2] = ITEM_COINS; //coins

    // Card 68  - Skyward Peaks
    deck[CARD_PEAKS].cardType = CARD_PEAKS;
    deck[CARD_PEAKS].numLinkedCards = 3;
    strcpy(deck[CARD_PEAKS].name, "Skyward Peaks");
    strcpy(deck[CARD_PEAKS].description, "Towering, snow capped mountains that pierce the sky,\ntheir jagged summits lost in the clouds.\nThe air is thin and cold, and the winds howl through the rocky passes.\nOnly the most daring travelers attempt the climb.");
    deck[CARD_PEAKS].linkedCards[0].cardType = CARD_VILLAGE; //?
    strcpy(deck[CARD_PEAKS].linkedCards[0].name,"east"); //link name
    deck[CARD_PEAKS].linkedCards[1].cardType = CARD_TOMBS; //?
    strcpy(deck[CARD_PEAKS].linkedCards[1].name,"south"); //link name
    deck[CARD_PEAKS].linkedCards[2].cardType = CARD_MOUNTAIN; //?
    strcpy(deck[CARD_PEAKS].linkedCards[2].name,"climb"); //link name

    // Card 69  - Ancient Glyphs
    deck[CARD_GLYPHS].cardType = CARD_GLYPHS;
    deck[CARD_GLYPHS].numLinkedCards = 2;
    deck[CARD_GLYPHS].numLinkedItems = 1;
    strcpy(deck[CARD_GLYPHS].name, "Ancient Glyphs");
    strcpy(deck[CARD_GLYPHS].description, "Faded symbols etched into stone, their meanings long lost to time.\nThey pulse faintly with an otherworldly energy,\nhinting at forgotten knowledge and hidden power,\nwaiting for someone to decipher their secrets.");
    deck[CARD_GLYPHS].linkedCards[0].cardType = CARD_VILLAGE; //?
    strcpy(deck[CARD_GLYPHS].linkedCards[0].name,"west"); //link name
    deck[CARD_GLYPHS].linkedCards[1].cardType = CARD_CAVERNS; //?
    strcpy(deck[CARD_GLYPHS].linkedCards[1].name,"south"); //link name
    deck[CARD_GLYPHS].linkedItems[0] = ITEM_BOOK; //book

    // Card 70  - Forsaken Road - Blue Wizard event
    deck[CARD_FORSAKEN_ROAD].cardType = CARD_FORSAKEN_ROAD;
    deck[CARD_FORSAKEN_ROAD].numLinkedCards = 1;
    deck[CARD_FORSAKEN_ROAD].numLinkedEnemies = 2;
    strcpy(deck[CARD_FORSAKEN_ROAD].name, "Forsaken Road");
    strcpy(deck[CARD_FORSAKEN_ROAD].description, "A desolate, overgrown path,\nonce traveled but now abandoned.\nCracked stone and twisted roots litter the way,\nand an eerie silence hangs in the air,\nas if the road itself has been forgotten by time.\nThe road continues north but a large chasm blocks you from continuing.");
    deck[CARD_FORSAKEN_ROAD].linkedCards[0].cardType = CARD_VILLAGE; //?
    strcpy(deck[CARD_FORSAKEN_ROAD].linkedCards[0].name,"south"); //link name
    deck[CARD_FORSAKEN_ROAD].linkedEnemies[0].enemyType = ENEMY_OGRE; //ogre
    deck[CARD_FORSAKEN_ROAD].linkedEnemies[0].health = 300; 
    deck[CARD_FORSAKEN_ROAD].linkedEnemies[0].damage = 80;
    deck[CARD_FORSAKEN_ROAD].linkedEnemies[1].enemyType = ENEMY_OGRE; //ogre
    deck[CARD_FORSAKEN_ROAD].linkedEnemies[1].health = 200; 
    deck[CARD_FORSAKEN_ROAD].linkedEnemies[1].damage = 80;

    // Card 71  - Hidden Passage
    deck[CARD_HIDDEN_PASSAGE].cardType = CARD_HIDDEN_PASSAGE;
    deck[CARD_HIDDEN_PASSAGE].numLinkedCards = 2;
    deck[CARD_HIDDEN_PASSAGE].numLinkedItems = 1;
    strcpy(deck[CARD_HIDDEN_PASSAGE].name, "Hidden Passage");
    strcpy(deck[CARD_HIDDEN_PASSAGE].description, "A secret, very long, narrow tunnel beneath the earth,\nits entrance concealed by rubble and roots.\nThe air is damp and musty,\nand the faint echo of footsteps suggests forgotten travelers once walked this path.\nYou can go east or west.");
    deck[CARD_HIDDEN_PASSAGE].linkedCards[0].cardType = CARD_CAVERNS; //?
    strcpy(deck[CARD_HIDDEN_PASSAGE].linkedCards[0].name,"west"); //link name
    deck[CARD_HIDDEN_PASSAGE].linkedCards[1].cardType = CARD_CAVE_DEEP; //?
    strcpy(deck[CARD_HIDDEN_PASSAGE].linkedCards[1].name,"east"); //link name
    deck[CARD_HIDDEN_PASSAGE].linkedItems[0] = ITEM_BOOK; //book

    // Card 72  - Enchanted Village Store
    deck[CARD_VILLAGE_STORE].cardType = CARD_VILLAGE_STORE;
    deck[CARD_VILLAGE_STORE].isStore = true;
    deck[CARD_VILLAGE_STORE].numLinkedCards = 1;
    strcpy(deck[CARD_VILLAGE_STORE].name, "Enchanted Village Store");
    strcpy(deck[CARD_VILLAGE_STORE].description, "You are in a small store.\nBuy items.");
    deck[CARD_VILLAGE_STORE].linkedCards[0].cardType = CARD_VILLAGE; //link back to village
    strcpy(deck[CARD_VILLAGE_STORE].linkedCards[0].name,"town"); //link name
    
    // Card 73  - Forsaken Road 2
    deck[CARD_FORSAKEN_ROAD_2].cardType = CARD_FORSAKEN_ROAD_2;
    deck[CARD_FORSAKEN_ROAD_2].numLinkedCards = 2;
    strcpy(deck[CARD_FORSAKEN_ROAD_2].name, "Forsaken Road North");
    strcpy(deck[CARD_FORSAKEN_ROAD_2].description, "You walk for miles along the northern end of the Forsaken Road.");
    deck[CARD_FORSAKEN_ROAD_2].linkedCards[0].cardType = CARD_FORSAKEN_ROAD; //?
    strcpy(deck[CARD_FORSAKEN_ROAD_2].linkedCards[0].name,"south"); //link name
    deck[CARD_FORSAKEN_ROAD_2].linkedCards[1].cardType = CARD_MARSH_ENTER; //?
    strcpy(deck[CARD_FORSAKEN_ROAD_2].linkedCards[1].name,"north"); //link name

    // Card 74  - Stoneheart Mountain
    deck[CARD_MOUNTAIN].cardType = CARD_MOUNTAIN;
    deck[CARD_MOUNTAIN].numLinkedCards = 1;
    deck[CARD_MOUNTAIN].numLinkedItems = 4;
    strcpy(deck[CARD_MOUNTAIN].name, "Stoneheart Mountain");
    strcpy(deck[CARD_MOUNTAIN].description, "You climb Stoneheart Mountain to the top.\nThe towering peak's slopes are blanketed in thick, glistening white drifts.\nThe jagged cliffs rise sharply against the sky,\ntheir dark stone contrasted by the endless stretch of frost and ice.\nWinds whip through the mountain's high passes,\ncarrying with them the bite of the cold\nIts too steep to climb back down.\nBut the south is less steep,\nand there is a path that leads down the mountain.");
    deck[CARD_MOUNTAIN].linkedCards[0].cardType = CARD_STACK_HOMESTEAD; //?
    strcpy(deck[CARD_MOUNTAIN].linkedCards[0].name,"path"); //link name
    deck[CARD_MOUNTAIN].linkedItems[0] = ITEM_HEALTH_POTION; //potion
    deck[CARD_MOUNTAIN].linkedItems[1] = ITEM_HEALTH_POTION; //potion
    deck[CARD_MOUNTAIN].linkedItems[2] = ITEM_MANA_POTION; //potion
    deck[CARD_MOUNTAIN].linkedItems[3] = ITEM_MANA_POTION; //potion

    // Card 75  - The Whispering Marsh Entrance
    deck[CARD_MARSH_ENTER].cardType = CARD_MARSH_ENTER;
    deck[CARD_MARSH_ENTER].numLinkedCards = 4;
    strcpy(deck[CARD_MARSH_ENTER].name, "The Whispering Marsh Entrance");
    strcpy(deck[CARD_MARSH_ENTER].description, "Entrance to The Whispering Marsh.\nIts rumored to be the home of ogres.\nEnter if you dare...");
    deck[CARD_MARSH_ENTER].linkedCards[0].cardType = CARD_MARSH_1; //?
    strcpy(deck[CARD_MARSH_ENTER].linkedCards[0].name,"north"); //link name
    deck[CARD_MARSH_ENTER].linkedCards[1].cardType = CARD_FORSAKEN_ROAD; //?
    strcpy(deck[CARD_MARSH_ENTER].linkedCards[1].name,"south"); //link name
    deck[CARD_MARSH_ENTER].linkedCards[2].cardType = CARD_MARSH_9; //?
    strcpy(deck[CARD_MARSH_ENTER].linkedCards[2].name,"east"); //link name
    deck[CARD_MARSH_ENTER].linkedCards[3].cardType = CARD_MARSH_3; //?
    strcpy(deck[CARD_MARSH_ENTER].linkedCards[3].name,"west"); //link name

    // Card 76  - The Whispering Marsh
    deck[CARD_MARSH_1].cardType = CARD_MARSH_1;
    deck[CARD_MARSH_1].numLinkedCards = 4;
    deck[CARD_MARSH_1].numLinkedEnemies = 4;
    strcpy(deck[CARD_MARSH_1].name, "The Whispering Marsh");
    strcpy(deck[CARD_MARSH_1].description, "A vast,\nfog-covered swamp where the wind seems to carry faint whispers from all directions.\nThe ground is treacherous,\nand strange,\nglowing lights float above the muck.");
    deck[CARD_MARSH_1].linkedCards[0].cardType = CARD_MARSH_2; //?
    strcpy(deck[CARD_MARSH_1].linkedCards[0].name,"north"); //link name
    deck[CARD_MARSH_1].linkedCards[1].cardType = CARD_MARSH_ENTER; //?
    strcpy(deck[CARD_MARSH_1].linkedCards[1].name,"south"); //link name
    deck[CARD_MARSH_1].linkedCards[2].cardType = CARD_MARSH_10; //?
    strcpy(deck[CARD_MARSH_1].linkedCards[2].name,"east"); //link name
    deck[CARD_MARSH_1].linkedCards[3].cardType = CARD_MARSH_4; //?
    strcpy(deck[CARD_MARSH_1].linkedCards[3].name,"west"); //link name
    deck[CARD_MARSH_1].linkedEnemies[0].enemyType = ENEMY_OGRE; //ogre
    deck[CARD_MARSH_1].linkedEnemies[0].health = 300; 
    deck[CARD_MARSH_1].linkedEnemies[0].damage = 80;
    deck[CARD_MARSH_1].linkedEnemies[1].enemyType = ENEMY_OGRE; //ogre
    deck[CARD_MARSH_1].linkedEnemies[1].health = 200; 
    deck[CARD_MARSH_1].linkedEnemies[1].damage = 80;
    deck[CARD_MARSH_1].linkedEnemies[2].enemyType = ENEMY_OGRE; //ogre
    deck[CARD_MARSH_1].linkedEnemies[2].health = 200; 
    deck[CARD_MARSH_1].linkedEnemies[2].damage = 80;
    deck[CARD_MARSH_1].linkedEnemies[3].enemyType = ENEMY_OGRE; //ogre
    deck[CARD_MARSH_1].linkedEnemies[3].health = 200; 
    deck[CARD_MARSH_1].linkedEnemies[3].damage = 80;

    // Card 77  - The Whispering Marsh
    deck[CARD_MARSH_2].cardType = CARD_MARSH_2;
    deck[CARD_MARSH_2].numLinkedCards = 1;
    strcpy(deck[CARD_MARSH_2].name, "The Whispering Marsh");
    strcpy(deck[CARD_MARSH_2].description, "A vast,\nfog-covered swamp where the wind seems to carry faint whispers from all directions.\nThe ground is treacherous,\nand strange,\nglowing lights float above the muck.");
    deck[CARD_MARSH_2].linkedCards[0].cardType = CARD_MARSH_1; //?
    strcpy(deck[CARD_MARSH_2].linkedCards[0].name,"south"); //link name

    // Card 78  - The Whispering Marsh
    deck[CARD_MARSH_3].cardType = CARD_MARSH_3;
    deck[CARD_MARSH_3].numLinkedCards = 3;
    deck[CARD_MARSH_3].numLinkedEnemies = 4;
    strcpy(deck[CARD_MARSH_3].name, "The Whispering Marsh");
    strcpy(deck[CARD_MARSH_3].description, "A vast,\nfog-covered swamp where the wind seems to carry faint whispers from all directions.\nThe ground is treacherous,\nand strange,\nglowing lights float above the muck.");
    deck[CARD_MARSH_3].linkedCards[0].cardType = CARD_MARSH_4; //?
    strcpy(deck[CARD_MARSH_3].linkedCards[0].name,"north"); //link name
    deck[CARD_MARSH_3].linkedCards[1].cardType = CARD_MARSH_ENTER; //?
    strcpy(deck[CARD_MARSH_3].linkedCards[1].name,"east"); //link name
    deck[CARD_MARSH_3].linkedCards[2].cardType = CARD_MARSH_6; //?
    strcpy(deck[CARD_MARSH_3].linkedCards[2].name,"west"); //link name
    deck[CARD_MARSH_3].linkedEnemies[0].enemyType = ENEMY_OGRE; //ogre
    deck[CARD_MARSH_3].linkedEnemies[0].health = 300; 
    deck[CARD_MARSH_3].linkedEnemies[0].damage = 80;
    deck[CARD_MARSH_3].linkedEnemies[1].enemyType = ENEMY_OGRE; //ogre
    deck[CARD_MARSH_3].linkedEnemies[1].health = 200; 
    deck[CARD_MARSH_3].linkedEnemies[1].damage = 80;
    deck[CARD_MARSH_3].linkedEnemies[2].enemyType = ENEMY_OGRE; //ogre
    deck[CARD_MARSH_3].linkedEnemies[2].health = 200; 
    deck[CARD_MARSH_3].linkedEnemies[2].damage = 80;
    deck[CARD_MARSH_3].linkedEnemies[3].enemyType = ENEMY_OGRE; //ogre
    deck[CARD_MARSH_3].linkedEnemies[3].health = 200; 
    deck[CARD_MARSH_3].linkedEnemies[3].damage = 80;

    // Card 79  - The Whispering Marsh
    deck[CARD_MARSH_4].cardType = CARD_MARSH_4;
    deck[CARD_MARSH_4].numLinkedCards = 4;
    deck[CARD_MARSH_4].numLinkedEnemies = 2;
    strcpy(deck[CARD_MARSH_4].name, "The Whispering Marsh");
    strcpy(deck[CARD_MARSH_4].description, "A vast,\nfog-covered swamp where the wind seems to carry faint whispers from all directions.\nThe ground is treacherous,\nand strange,\nglowing lights float above the muck.");
    deck[CARD_MARSH_4].linkedCards[0].cardType = CARD_MARSH_5; //?
    strcpy(deck[CARD_MARSH_4].linkedCards[0].name,"north"); //link name
    deck[CARD_MARSH_4].linkedCards[1].cardType = CARD_MARSH_3; //?
    strcpy(deck[CARD_MARSH_4].linkedCards[1].name,"south"); //link name
    deck[CARD_MARSH_4].linkedCards[2].cardType = CARD_MARSH_1; //?
    strcpy(deck[CARD_MARSH_4].linkedCards[2].name,"east"); //link name
    deck[CARD_MARSH_4].linkedCards[3].cardType = CARD_MARSH_7; //?
    strcpy(deck[CARD_MARSH_4].linkedCards[3].name,"west"); //link name
    deck[CARD_MARSH_4].linkedEnemies[0].enemyType = ENEMY_OGRE; //ogre
    deck[CARD_MARSH_4].linkedEnemies[0].health = 300; 
    deck[CARD_MARSH_4].linkedEnemies[0].damage = 80;
    deck[CARD_MARSH_4].linkedEnemies[1].enemyType = ENEMY_OGRE; //ogre
    deck[CARD_MARSH_4].linkedEnemies[1].health = 200; 
    deck[CARD_MARSH_4].linkedEnemies[1].damage = 80;

    // Card 80  - The Whispering Marsh
    deck[CARD_MARSH_5].cardType = CARD_MARSH_5;
    deck[CARD_MARSH_5].numLinkedCards = 2;
    strcpy(deck[CARD_MARSH_5].name, "The Whispering Marsh");
    strcpy(deck[CARD_MARSH_5].description, "A vast,\nfog-covered swamp where the wind seems to carry faint whispers from all directions.\nThe ground is treacherous,\nand strange,\nglowing lights float above the muck.");
    deck[CARD_MARSH_5].linkedCards[0].cardType = CARD_MARSH_EXIT; //?
    strcpy(deck[CARD_MARSH_5].linkedCards[0].name,"out"); //link name
    deck[CARD_MARSH_5].linkedCards[1].cardType = CARD_MARSH_4; //?
    strcpy(deck[CARD_MARSH_5].linkedCards[1].name,"south"); //link name

    // Card 81  - The Whispering Marsh
    deck[CARD_MARSH_6].cardType = CARD_MARSH_6;
    deck[CARD_MARSH_6].numLinkedCards = 2;
    deck[CARD_MARSH_6].numLinkedEnemies = 2;
    strcpy(deck[CARD_MARSH_6].name, "The Whispering Marsh");
    strcpy(deck[CARD_MARSH_6].description, "A vast,\nfog-covered swamp where the wind seems to carry faint whispers from all directions.\nThe ground is treacherous,\nand strange,\nglowing lights float above the muck.");
    deck[CARD_MARSH_6].linkedCards[0].cardType = CARD_MARSH_7; //?
    strcpy(deck[CARD_MARSH_6].linkedCards[0].name,"north"); //link name
    deck[CARD_MARSH_6].linkedCards[1].cardType = CARD_MARSH_3; //?
    strcpy(deck[CARD_MARSH_6].linkedCards[1].name,"east"); //link name
    deck[CARD_MARSH_6].linkedEnemies[0].enemyType = ENEMY_OGRE; //ogre
    deck[CARD_MARSH_6].linkedEnemies[0].health = 300; 
    deck[CARD_MARSH_6].linkedEnemies[0].damage = 80;
    deck[CARD_MARSH_6].linkedEnemies[1].enemyType = ENEMY_OGRE; //ogre
    deck[CARD_MARSH_6].linkedEnemies[1].health = 200; 
    deck[CARD_MARSH_6].linkedEnemies[1].damage = 80;

    // Card 82  - The Whispering Marsh
    deck[CARD_MARSH_7].cardType = CARD_MARSH_7;
    deck[CARD_MARSH_7].numLinkedCards = 2;
    deck[CARD_MARSH_7].numLinkedEnemies = 1;
    strcpy(deck[CARD_MARSH_7].name, "The Whispering Marsh");
    strcpy(deck[CARD_MARSH_7].description, "A vast,\nfog-covered swamp where the wind seems to carry faint whispers from all directions.\nThe ground is treacherous,\nand strange,\nglowing lights float above the muck.");
    deck[CARD_MARSH_7].linkedCards[0].cardType = CARD_MARSH_6; //?
    strcpy(deck[CARD_MARSH_7].linkedCards[0].name,"south"); //link name
    deck[CARD_MARSH_7].linkedCards[1].cardType = CARD_MARSH_4; //?
    strcpy(deck[CARD_MARSH_7].linkedCards[1].name,"east"); //link name
    deck[CARD_MARSH_7].linkedEnemies[0].enemyType = ENEMY_OGRE; //ogre
    deck[CARD_MARSH_7].linkedEnemies[0].health = 400; 
    deck[CARD_MARSH_7].linkedEnemies[0].damage = 110;

    // Card 83  - The Whispering Marsh
    deck[CARD_MARSH_8].cardType = CARD_MARSH_8;
    deck[CARD_MARSH_8].numLinkedCards = 2;
    strcpy(deck[CARD_MARSH_8].name, "The Whispering Marsh");
    strcpy(deck[CARD_MARSH_8].description, "A vast,\nfog-covered swamp where the wind seems to carry faint whispers from all directions.\nThe ground is treacherous,\nand strange,\nglowing lights float above the muck.");
    deck[CARD_MARSH_8].linkedCards[0].cardType = CARD_MARSH_9; //?
    strcpy(deck[CARD_MARSH_8].linkedCards[0].name,"north"); //link name
    deck[CARD_MARSH_8].linkedCards[1].cardType = CARD_MARSH_11; //?
    strcpy(deck[CARD_MARSH_8].linkedCards[1].name,"east"); //link name

    // Card 84  - The Whispering Marsh
    deck[CARD_MARSH_9].cardType = CARD_MARSH_9;
    deck[CARD_MARSH_9].numLinkedCards = 4;
    strcpy(deck[CARD_MARSH_9].name, "The Whispering Marsh");
    strcpy(deck[CARD_MARSH_9].description, "A vast,\nfog-covered swamp where the wind seems to carry faint whispers from all directions.\nThe ground is treacherous,\nand strange,\nglowing lights float above the muck.");
    deck[CARD_MARSH_9].linkedCards[0].cardType = CARD_MARSH_10; //?
    strcpy(deck[CARD_MARSH_9].linkedCards[0].name,"north"); //link name
    deck[CARD_MARSH_9].linkedCards[1].cardType = CARD_MARSH_8; //?
    strcpy(deck[CARD_MARSH_9].linkedCards[1].name,"south"); //link name
    deck[CARD_MARSH_9].linkedCards[2].cardType = CARD_MARSH_12; //?
    strcpy(deck[CARD_MARSH_9].linkedCards[2].name,"east"); //link name
    deck[CARD_MARSH_9].linkedCards[3].cardType = CARD_MARSH_ENTER; //?
    strcpy(deck[CARD_MARSH_9].linkedCards[3].name,"west"); //link name

    // Card 85  - The Whispering Marsh
    deck[CARD_MARSH_10].cardType = CARD_MARSH_10;
    deck[CARD_MARSH_10].numLinkedCards = 3;
    deck[CARD_MARSH_10].numLinkedEnemies = 4;
    strcpy(deck[CARD_MARSH_10].name, "The Whispering Marsh");
    strcpy(deck[CARD_MARSH_10].description, "A vast,\nfog-covered swamp where the wind seems to carry faint whispers from all directions.\nThe ground is treacherous,\nand strange,\nglowing lights float above the muck.");
    deck[CARD_MARSH_10].linkedCards[0].cardType = CARD_MARSH_9; //?
    strcpy(deck[CARD_MARSH_10].linkedCards[0].name,"south"); //link name
    deck[CARD_MARSH_10].linkedCards[1].cardType = CARD_MARSH_13; //?
    strcpy(deck[CARD_MARSH_10].linkedCards[1].name,"east"); //link name
    deck[CARD_MARSH_10].linkedCards[2].cardType = CARD_MARSH_1; //?
    strcpy(deck[CARD_MARSH_10].linkedCards[2].name,"west"); //link name
    deck[CARD_MARSH_10].linkedEnemies[0].enemyType = ENEMY_OGRE; //ogre
    deck[CARD_MARSH_10].linkedEnemies[0].health = 300; 
    deck[CARD_MARSH_10].linkedEnemies[0].damage = 80;
    deck[CARD_MARSH_10].linkedEnemies[1].enemyType = ENEMY_OGRE; //ogre
    deck[CARD_MARSH_10].linkedEnemies[1].health = 200; 
    deck[CARD_MARSH_10].linkedEnemies[1].damage = 80;
    deck[CARD_MARSH_10].linkedEnemies[2].enemyType = ENEMY_OGRE; //ogre
    deck[CARD_MARSH_10].linkedEnemies[2].health = 300; 
    deck[CARD_MARSH_10].linkedEnemies[2].damage = 80;
    deck[CARD_MARSH_10].linkedEnemies[3].enemyType = ENEMY_OGRE; //ogre
    deck[CARD_MARSH_10].linkedEnemies[3].health = 200; 
    deck[CARD_MARSH_10].linkedEnemies[3].damage = 80;

    // Card 86  - The Whispering Marsh
    deck[CARD_MARSH_11].cardType = CARD_MARSH_11;
    deck[CARD_MARSH_11].numLinkedCards = 2;
    deck[CARD_MARSH_11].numLinkedItems = 1;
    strcpy(deck[CARD_MARSH_11].name, "The Whispering Marsh");
    strcpy(deck[CARD_MARSH_11].description, "A vast,\nfog-covered swamp where the wind seems to carry faint whispers from all directions.\nThe ground is treacherous,\nand strange,\nglowing lights float above the muck.");
    deck[CARD_MARSH_11].linkedCards[0].cardType = CARD_MARSH_12; //?
    strcpy(deck[CARD_MARSH_11].linkedCards[0].name,"north"); //link name
    deck[CARD_MARSH_11].linkedCards[1].cardType = CARD_MARSH_8; //?
    strcpy(deck[CARD_MARSH_11].linkedCards[1].name,"west"); //link name
    deck[CARD_MARSH_11].linkedItems[0] = ITEM_BOOK; //book

    // Card 87  - The Whispering Marsh
    deck[CARD_MARSH_12].cardType = CARD_MARSH_12;
    deck[CARD_MARSH_12].numLinkedCards = 3;
    deck[CARD_MARSH_12].numLinkedEnemies = 4;
    strcpy(deck[CARD_MARSH_12].name, "The Whispering Marsh");
    strcpy(deck[CARD_MARSH_12].description, "A vast,\nfog-covered swamp where the wind seems to carry faint whispers from all directions.\nThe ground is treacherous,\nand strange,\nglowing lights float above the muck.");
    deck[CARD_MARSH_12].linkedCards[0].cardType = CARD_MARSH_13; //?
    strcpy(deck[CARD_MARSH_12].linkedCards[0].name,"north"); //link name
    deck[CARD_MARSH_12].linkedCards[1].cardType = CARD_MARSH_11; //?
    strcpy(deck[CARD_MARSH_12].linkedCards[1].name,"south"); //link name
    deck[CARD_MARSH_12].linkedCards[2].cardType = CARD_MARSH_9; //?
    strcpy(deck[CARD_MARSH_12].linkedCards[2].name,"west"); //link name
    deck[CARD_MARSH_12].linkedEnemies[0].enemyType = ENEMY_OGRE; //ogre
    deck[CARD_MARSH_12].linkedEnemies[0].health = 300; 
    deck[CARD_MARSH_12].linkedEnemies[0].damage = 80;
    deck[CARD_MARSH_12].linkedEnemies[1].enemyType = ENEMY_OGRE; //ogre
    deck[CARD_MARSH_12].linkedEnemies[1].health = 200; 
    deck[CARD_MARSH_12].linkedEnemies[1].damage = 80;
    deck[CARD_MARSH_12].linkedEnemies[2].enemyType = ENEMY_OGRE; //ogre
    deck[CARD_MARSH_12].linkedEnemies[2].health = 300; 
    deck[CARD_MARSH_12].linkedEnemies[2].damage = 80;
    deck[CARD_MARSH_12].linkedEnemies[3].enemyType = ENEMY_OGRE; //ogre
    deck[CARD_MARSH_12].linkedEnemies[3].health = 200; 
    deck[CARD_MARSH_12].linkedEnemies[3].damage = 80;

    // Card 88  - The Whispering Marsh
    deck[CARD_MARSH_13].cardType = CARD_MARSH_13;
    deck[CARD_MARSH_13].numLinkedCards = 2;
    deck[CARD_MARSH_13].numLinkedEnemies = 1;
    strcpy(deck[CARD_MARSH_13].name, "The Whispering Marsh");
    strcpy(deck[CARD_MARSH_13].description, "A vast,\nfog-covered swamp where the wind seems to carry faint whispers from all directions.\nThe ground is treacherous,\nand strange,\nglowing lights float above the muck.");
    deck[CARD_MARSH_13].linkedCards[0].cardType = CARD_MARSH_12; //?
    strcpy(deck[CARD_MARSH_13].linkedCards[0].name,"south"); //link name
    deck[CARD_MARSH_13].linkedCards[1].cardType = CARD_MARSH_10; //?
    strcpy(deck[CARD_MARSH_13].linkedCards[1].name,"west"); //link name
    deck[CARD_MARSH_13].linkedEnemies[0].enemyType = ENEMY_OGRE; //ogre
    deck[CARD_MARSH_13].linkedEnemies[0].health = 500; 
    deck[CARD_MARSH_13].linkedEnemies[0].damage = 160;

    // Card 89  - The Whispering Marsh Exit
    deck[CARD_MARSH_EXIT].cardType = CARD_MARSH_EXIT;
    deck[CARD_MARSH_EXIT].numLinkedCards = 2;
    strcpy(deck[CARD_MARSH_EXIT].name, "The Whispering Marsh Exit");
    strcpy(deck[CARD_MARSH_EXIT].description, "You can head south back into the marsh, or north...");
    deck[CARD_MARSH_EXIT].linkedCards[0].cardType = CARD_MARSH_5; //?
    strcpy(deck[CARD_MARSH_EXIT].linkedCards[0].name,"south"); //link name
    deck[CARD_MARSH_EXIT].linkedCards[1].cardType = CARD_HEARTHSTONE; //?
    strcpy(deck[CARD_MARSH_EXIT].linkedCards[1].name,"north"); //link name

    // Card 90  - Town of Hearthstone
    deck[CARD_HEARTHSTONE].cardType = CARD_HEARTHSTONE;
    deck[CARD_HEARTHSTONE].numLinkedCards = 6;
    deck[CARD_HEARTHSTONE].numLinkedItems = 4;
    strcpy(deck[CARD_HEARTHSTONE].name, "Town of Hearthstone");
    strcpy(deck[CARD_HEARTHSTONE].description, "Hearthstone is a charming town nestled in a peaceful valley.\nWith its cozy cottages, lush farmlands, and scenic countryside, it exudes a warm, welcoming atmosphere.\nThe quiet streets are lined with flower-filled gardens,\nand the air is fresh with the scent of nearby forests and fields.");
    deck[CARD_HEARTHSTONE].linkedCards[0].cardType = CARD_COTTAGE_2; //?
    strcpy(deck[CARD_HEARTHSTONE].linkedCards[0].name,"north"); //link name
    deck[CARD_HEARTHSTONE].linkedCards[1].cardType = CARD_MARSH_EXIT; //?
    strcpy(deck[CARD_HEARTHSTONE].linkedCards[1].name,"south"); //link name
    deck[CARD_HEARTHSTONE].linkedCards[2].cardType = CARD_HEARTHSTONE_ROAD; //?
    strcpy(deck[CARD_HEARTHSTONE].linkedCards[2].name,"east"); //link name
    deck[CARD_HEARTHSTONE].linkedCards[3].cardType = CARD_COTTAGE_1; //?
    strcpy(deck[CARD_HEARTHSTONE].linkedCards[3].name,"west"); //link name
    deck[CARD_HEARTHSTONE].linkedCards[4].cardType = CARD_AJ; //?
    strcpy(deck[CARD_HEARTHSTONE].linkedCards[4].name,"path"); //link name
    deck[CARD_HEARTHSTONE].linkedCards[5].cardType = CARD_HEARTHSTONE_STORE; //?
    strcpy(deck[CARD_HEARTHSTONE].linkedCards[5].name,"store"); //link name
    deck[CARD_HEARTHSTONE].linkedItems[0] = ITEM_HEALTH_POTION; //potion
    deck[CARD_HEARTHSTONE].linkedItems[1] = ITEM_HEALTH_POTION; //potion
    deck[CARD_HEARTHSTONE].linkedItems[2] = ITEM_MANA_POTION; //potion
    deck[CARD_HEARTHSTONE].linkedItems[3] = ITEM_MANA_POTION; //potion

    // Card 91  - Willow and Wares
    deck[CARD_HEARTHSTONE_STORE].cardType = CARD_HEARTHSTONE_STORE;
    deck[CARD_HEARTHSTONE_STORE].numLinkedCards = 1;
    deck[CARD_HEARTHSTONE_STORE].isStore = true;
    strcpy(deck[CARD_HEARTHSTONE_STORE].name, "Willow and Wares");
    strcpy(deck[CARD_HEARTHSTONE_STORE].description, "You are in a small store.\nBuy items.");
    deck[CARD_HEARTHSTONE_STORE].linkedCards[0].cardType = CARD_HEARTHSTONE; //?
    strcpy(deck[CARD_HEARTHSTONE_STORE].linkedCards[0].name,"town"); //link name

    // Card 92  - Thistlehill Cottage
    deck[CARD_COTTAGE_1].cardType = CARD_COTTAGE_1;
    deck[CARD_COTTAGE_1].numLinkedCards = 2;
    deck[CARD_COTTAGE_1].numLinkedItems = 2;
    strcpy(deck[CARD_COTTAGE_1].name, "Thistlehill Cottage");
    strcpy(deck[CARD_COTTAGE_1].description, "Thistlehill Cottage is a charming little retreat perched on a hillside,\nsurrounded by wildflowers and the gentle rustle of the breeze.");
    deck[CARD_COTTAGE_1].linkedCards[0].cardType = CARD_FARM_BAKER; //?
    strcpy(deck[CARD_COTTAGE_1].linkedCards[0].name,"north"); //link name
    deck[CARD_COTTAGE_1].linkedCards[1].cardType = CARD_HEARTHSTONE; //?
    strcpy(deck[CARD_COTTAGE_1].linkedCards[1].name,"east"); //link name
    deck[CARD_COTTAGE_1].linkedItems[0] = ITEM_HEALTH_POTION; //potion
    deck[CARD_COTTAGE_1].linkedItems[1] = ITEM_HEALTH_POTION; //potion

    // Card 93  - Sunbeam Cottage
    deck[CARD_COTTAGE_2].cardType = CARD_COTTAGE_2;
    deck[CARD_COTTAGE_2].numLinkedCards = 2;
    deck[CARD_COTTAGE_2].numLinkedItems = 2;
    strcpy(deck[CARD_COTTAGE_2].name, "Sunbeam Cottage");
    strcpy(deck[CARD_COTTAGE_2].description, "A bright, welcoming home bathed in natural light.\nWith its large windows that let the sun spill in,\nthe cottage feels warm and cheerful all year round.\nInside are soft wooden beams and colorful, hand-crafted decor");
    deck[CARD_COTTAGE_2].linkedCards[0].cardType = CARD_HEARTHSTONE; //?
    strcpy(deck[CARD_COTTAGE_2].linkedCards[0].name,"south"); //link name
    deck[CARD_COTTAGE_2].linkedCards[1].cardType = CARD_FARM_BAKER; //?
    strcpy(deck[CARD_COTTAGE_2].linkedCards[1].name,"west"); //link name
    deck[CARD_COTTAGE_2].linkedItems[0] = ITEM_HEALTH_POTION; //potion
    deck[CARD_COTTAGE_2].linkedItems[1] = ITEM_HEALTH_POTION; //potion

    // Card 94  - Baker Farm
    deck[CARD_FARM_BAKER].cardType = CARD_FARM_BAKER;
    deck[CARD_FARM_BAKER].numLinkedCards = 3;
    deck[CARD_FARM_BAKER].numLinkedItems = 1;
    strcpy(deck[CARD_FARM_BAKER].name, "Baker Farm");
    strcpy(deck[CARD_FARM_BAKER].description, "The Baker Farm is a cozy,\npicturesque homestead nestled at the edge of Hearthstone,\nwhere the sweet scent of fresh bread and earthy fields mingle in the air.\nThe farmhouse,\nwith its red-brick chimney and whitewashed walls,\nstands proudly beside neat rows of crops and flourishing orchards.\nThere is a beet plant here.");
    deck[CARD_FARM_BAKER].linkedCards[0].cardType = CARD_COTTAGE_1; //?
    strcpy(deck[CARD_FARM_BAKER].linkedCards[0].name,"south"); //link name
    deck[CARD_FARM_BAKER].linkedCards[1].cardType = CARD_COTTAGE_2; //?
    strcpy(deck[CARD_FARM_BAKER].linkedCards[1].name,"east"); //link name
    deck[CARD_FARM_BAKER].linkedCards[2].cardType = CARD_FARM_TURNEY; //?
    strcpy(deck[CARD_FARM_BAKER].linkedCards[2].name,"west"); //link name
    deck[CARD_FARM_BAKER].linkedItems[0] = ITEM_BOOK; //book
    

    // Card 95  - Old Man Aj's
    deck[CARD_AJ].cardType = CARD_AJ;
    deck[CARD_AJ].numLinkedCards = 1;
    deck[CARD_AJ].numLinkedItems = 4;
    strcpy(deck[CARD_AJ].name, "Old Man Aj's");
    strcpy(deck[CARD_AJ].description, "Old Man Aj's house is a weathered,\nyet sturdy cottage nestled at the edge of Hearthstone.\nIts timbered walls are worn with age, and the roof, patched in places, is thick with moss.\nThe front porch is cluttered with an assortment of tools, jars of dried herbs, and an odd collection of knick-knacks.\nThere is a fireplace here.");
    deck[CARD_AJ].linkedCards[0].cardType = CARD_HEARTHSTONE; //?
    strcpy(deck[CARD_AJ].linkedCards[0].name,"path"); //link name
    deck[CARD_AJ].linkedItems[0] = ITEM_MANA_POTION; //potion
    deck[CARD_AJ].linkedItems[1] = ITEM_MANA_POTION; //potion
    deck[CARD_AJ].linkedItems[2] = ITEM_MANA_POTION; //potion
    deck[CARD_AJ].linkedItems[3] = ITEM_MANA_POTION; //potion

    // Card 96  - Road near Hearthstone
    deck[CARD_HEARTHSTONE_ROAD].cardType = CARD_HEARTHSTONE_ROAD;
    deck[CARD_HEARTHSTONE_ROAD].numLinkedCards = 2;
    strcpy(deck[CARD_HEARTHSTONE_ROAD].name, "Road near Hearthstone");
    strcpy(deck[CARD_HEARTHSTONE_ROAD].description, "The Road near Hearthstone winds gently through lush meadows and wooded glades,\noffering peaceful views of the surrounding countryside.\nPaved with cobblestones that have worn smooth over the years,\nit's flanked by wildflowers and tall grasses that sway in the breeze.");
    deck[CARD_HEARTHSTONE_ROAD].linkedCards[0].cardType = CARD_HEARTHSTONE; //?
    strcpy(deck[CARD_HEARTHSTONE_ROAD].linkedCards[0].name,"west"); //link name
    deck[CARD_HEARTHSTONE_ROAD].linkedCards[1].cardType = CARD_PUB; //?
    strcpy(deck[CARD_HEARTHSTONE_ROAD].linkedCards[1].name,"pub"); //link name

    // Card 97  - Turney Farm
    deck[CARD_FARM_TURNEY].cardType = CARD_FARM_TURNEY;
    deck[CARD_FARM_TURNEY].numLinkedCards = 1;
    deck[CARD_FARM_TURNEY].numLinkedItems = 1;
    strcpy(deck[CARD_FARM_TURNEY].name, "Turney Farm");
    strcpy(deck[CARD_FARM_TURNEY].description, "A nice, quiet, farm house in the country-side.\nLooks like a good place for a read.");
    deck[CARD_FARM_TURNEY].linkedCards[0].cardType = CARD_FARM_BAKER; //?
    strcpy(deck[CARD_FARM_TURNEY].linkedCards[0].name,"east"); //link name
    deck[CARD_FARM_TURNEY].linkedItems[0] = ITEM_BOOK; //book

    // Card 98  - Pub
    deck[CARD_PUB].cardType = CARD_PUB;
    deck[CARD_PUB].numLinkedCards = 1;
    strcpy(deck[CARD_PUB].name, "The Stumbling Goat");
    strcpy(deck[CARD_PUB].description, "A humorous, rustic tavern with a quirky feel, often the heart of village gossip.");
    deck[CARD_PUB].linkedCards[0].cardType = CARD_HEARTHSTONE_ROAD; //?
    strcpy(deck[CARD_PUB].linkedCards[0].name,"road"); //link name

    // Card 99  - Road east of Wrenwood
    deck[CARD_WRENWOOD_ROAD].cardType = CARD_WRENWOOD_ROAD;
    deck[CARD_WRENWOOD_ROAD].numLinkedCards = 2;
    deck[CARD_WRENWOOD_ROAD].numLinkedEnemies = 1;
    strcpy(deck[CARD_WRENWOOD_ROAD].name, "Road east of Wrenwood");
    strcpy(deck[CARD_WRENWOOD_ROAD].description, "You travel miles east of Wrenwood.\nIt looks like another town is nearby");
    deck[CARD_WRENWOOD_ROAD].linkedCards[0].cardType = CARD_BELLEVILLE; //?
    strcpy(deck[CARD_WRENWOOD_ROAD].linkedCards[0].name,"east"); //link name
    deck[CARD_WRENWOOD_ROAD].linkedCards[1].cardType = CARD_WRENWOOD; //?
    strcpy(deck[CARD_WRENWOOD_ROAD].linkedCards[1].name,"west"); //link name
    deck[CARD_WRENWOOD_ROAD].linkedEnemies[0].enemyType = ENEMY_ORC; //orc
    deck[CARD_WRENWOOD_ROAD].linkedEnemies[0].health = 1000; 
    deck[CARD_WRENWOOD_ROAD].linkedEnemies[0].damage = 100;

    // Card 100  - Belleville
    deck[CARD_BELLEVILLE].cardType = CARD_BELLEVILLE;
    deck[CARD_BELLEVILLE].numLinkedCards = 3;
    strcpy(deck[CARD_BELLEVILLE].name, "Belleville");
    strcpy(deck[CARD_BELLEVILLE].description, "At the center of Belleville lies a modest square,\ndominated by an ancient oak tree whose gnarled branches stretch wide,\ncasting dappled shadows over the market stalls below.");
    deck[CARD_BELLEVILLE].linkedCards[0].cardType = CARD_BELLEVILLE_LIBRARY; //?
    strcpy(deck[CARD_BELLEVILLE].linkedCards[0].name,"books"); //link name
    deck[CARD_BELLEVILLE].linkedCards[1].cardType = CARD_BELLEVILLE_STORE; //?
    strcpy(deck[CARD_BELLEVILLE].linkedCards[1].name,"store"); //link name
    deck[CARD_BELLEVILLE].linkedCards[2].cardType = CARD_WRENWOOD_ROAD; //?
    strcpy(deck[CARD_BELLEVILLE].linkedCards[2].name,"west"); //link name

    // Card 101  - Belleville Store
    deck[CARD_BELLEVILLE_STORE].cardType = CARD_BELLEVILLE_STORE;
    deck[CARD_BELLEVILLE_STORE].numLinkedCards = 1;
    deck[CARD_BELLEVILLE_STORE].isStore = true;
    strcpy(deck[CARD_BELLEVILLE_STORE].name, "Ye Ol'e Dog House");
    strcpy(deck[CARD_BELLEVILLE_STORE].description, "A quaint small shop known for its ice cream.");
    deck[CARD_BELLEVILLE_STORE].linkedCards[0].cardType = CARD_BELLEVILLE; //?
    strcpy(deck[CARD_BELLEVILLE_STORE].linkedCards[0].name,"town"); //link name

    // Card 102  - Belleville Library - has fireplace
    deck[CARD_BELLEVILLE_LIBRARY].cardType = CARD_BELLEVILLE_LIBRARY;
    deck[CARD_BELLEVILLE_LIBRARY].numLinkedCards = 1;
    deck[CARD_BELLEVILLE_LIBRARY].numLinkedItems = 1;
    strcpy(deck[CARD_BELLEVILLE_LIBRARY].name, "Belleville Public Library");
    strcpy(deck[CARD_BELLEVILLE_LIBRARY].description, "A public library for old and young.\nA good place for a read.\nThere is a fireplace here.");
    deck[CARD_BELLEVILLE_LIBRARY].linkedCards[0].cardType = CARD_BELLEVILLE; //?
    strcpy(deck[CARD_BELLEVILLE_LIBRARY].linkedCards[0].name,"town"); //link name
    deck[CARD_BELLEVILLE_LIBRARY].linkedItems[0] = ITEM_BOOK;
 


    // Set the starting room
    currentCard = &deck[CARD_HOME];
}


//remove trailing new lines and convert uppercase to lowercase
void formatInput(char* input)
{
    // Remove trailing newline
    input[strcspn(input, "\n")] = '\0';
    //convert input to lowercase
    for (int i = 0; i < MAX_INPUT_UNIQUE; i++)
    {
        if (input[i] <= 'Z' || input[i] >= 'A')
        {
            input[i] = tolower((char)input[i]);
        }
    }
}

//handle the finding of items
void printAndHandleItem(int itemType)
{
    // switch statement
    switch (itemType)
    {
    case ITEM_SWORD:
        printf("Got the sword\n");
        hasSword = true;
        break;

    case ITEM_MAP:
        printf("Got the map\n");
        hasMap = true;
        break;

    case ITEM_HEALTH_POTION:
        printf("Found health potion\n");
        health += 100;
        break;
    
    case ITEM_MANA_POTION:
        printf("Found mana potion\n");
        mana += 100;
        break;

    case ITEM_COINS:
        printf("Found some coins\n");
        money += 100;
        break;

    case ITEM_BOOK:
        printf("Found a book\n");
        books += 1;
        break;

    default:
        printf("Did not recognize item type\n");
        break;
    }
}

//return the correct enemy name string from the enum value
char* getEnemyName(int enemyType)
{
    static char* result;
    // switch statement
    switch (enemyType)
    {
    case ENEMY_OGRE:
        result =  "ogre";
        break;

    case ENEMY_WOLF:
        result =  "wolf";
        break;

    case ENEMY_BEAR:
        result =  "bear";
        break;
    
    case ENEMY_GOBLIN:
        result =  "goblin";
        break;

    case ENEMY_ORC:
        result =  "orc";
        break;

    default:
        result = "(n/a)";
        break;
    }
    return result;
}

//return the correct enemy attack name string from the enum value
char* getEnemyAttack(int enemyType)
{
    static char* result;
    // switch statement
    switch (enemyType)
    {
    case ENEMY_OGRE:
        result =  "hits";
        break;

    case ENEMY_WOLF:
        result =  "bites";
        break;

    case ENEMY_BEAR:
        result =  "bites";
        break;
    
    case ENEMY_GOBLIN:
        result =  "stabs";
        break;

    case ENEMY_ORC:
        result =  "stabs";
        break;

    default:
        result = "(n/a)";
        break;
    }
    return result;
}

//handle the fighting
void handleFightSequence()
{
    for(int i = 0; i < currentCard->numLinkedEnemies; i++)
    {
        if(currentCard->linkedEnemies[i].health > 0)
        {
            int type = currentCard->linkedEnemies[i].enemyType;
            int damage = currentCard->linkedEnemies[i].damage;
            printf("the %s %s you, loss of %d health\n", getEnemyName(type), getEnemyAttack(type), damage);
            health -= damage;
        }//else its dead and we dont care
    }

    if(health <= 0)
    {
        printf("\n\nyou died...\n\n");//death in this game is like grand theft auto sort of...
        // Set the starting room back to home
        currentCard = &deck[CARD_HOME];
        health = 100;//set starting health to 100, you will have to regain a surplus
    }
}

//truly, if there is an example of how much this is just all spaghetti code, this is it...
//max damage = 1000, at 9000 xp
int getDamage()
{
    if(xp > 9000)
    {
        return 1000;//max damage
    }
    else if(xp > 800)
    {
        return 900;
    }
    else if(xp > 7000)
    {
        return 800;
    }
    else if(xp > 6000)
    {
        return 700;
    }
    else if(xp > 5000)
    {
        return 600;
    }
    else if(xp > 4000)
    {
        return 500;
    }
    else if(xp > 3000)
    {
        return 400;
    }
    else if(xp > 2000)
    {
        return 300;
    }
    else if(xp > 1000)
    {
        return 200;
    }
    else if(xp > 500)
    {
        return 150;
    }
    else if(xp > 200)
    {
        return 120;
    }
    else if(xp > 100)
    {
        return 100;
    }
    else if(xp > 50)
    {
        return 50;
    }
    else if(xp > 40)
    {
        return 40;
    }
    else if(xp > 30)
    {
        return 30;
    }
    else
    {
        return 20;
    }
}

//event handler
void handleEvents()
{
    if(currentCard->cardType == CARD_HOME && !ringEventHasHappened && loops > 10)
    {
        ringEventHasHappened = true;
        printf("\nThere is a knock at the door...\n");
        printf("You open the door and standing there is the Blue Wizard.\n");
        printf("He hands you a magic ring, and then he leaves.\n\n");
        printf("You got the ring\n");
        hasRing = true;
    }
    else if(currentCard->cardType == CARD_TOWN_SQUARE && !kingEventHasHappened && loops > 50)
    {
        kingEventHasHappened = true;
        printf("\nThe king arrives...\n");
        printf("Trumpets blare to announce his arrival.\n");
        printf("There is a large crowd gathered to hear him speak.\n");
        printf("But as he begins to speak, he looks at you and says...\n");
        printf("'I am the king of this land, and though I wear the crown, it is you who may be its true heir. The trials\n");
        printf("ahead will test your resolve, your courage, and your very soul. Do you have the strength to rise above? I \n");
        printf("believe you do. Now go forth, and prove that you are worthy of the legend that awaits.'\n");
        if(hasMap)
        {
            printf("The King says 'I see you have the map, I trust you can make your way.'\n");
            xp += 50;
            printf("(received 50 experience points)\n");
        }
        printf("The king leaves and the crowd disbands.\n\n");
    }
    else if(currentCard->cardType == CARD_WILD_GARDEN && !tomBomEventHasHappened && loops > 25)
    {
        tomBomEventHasHappened = true;
        printf("\nA strange man walks out of the forest, he is singing:\n");
        printf("'Oh, the rivers so wide and the hills are so high,\n");
        printf("I sing to the birds and I dance with the sky.\n");
        printf("The trees in the forest are singing with me,\n");
        printf("The flowers all bloom where I want them to be!\n");
        printf("Old Tom Bombadil, hey, ho!\n");
        printf("Laughing and skipping wherever I go!\n");
        printf("With a hey, ho! and a dance, so free,'\n");
        printf("The world is just right when youre wild like me!'\n");
        printf("\n\nHe stops singing and looks at you, and says\n");
        printf("'Well now, who do we have here, wandering through the woods? A curious one, you are! No need to \n");
        printf("fret, youre in good hands with Old Tom Bombadil. The trees and the streams know me well, and \n");
        printf("theyll tell you that no harm comes when youre with me. Where youve been, where youre going\n");
        printf("those dont matter much. All that matters is the now!'\n");
        printf("\n");
        printf("Tom Bombadil walks back into the forest.\n\n");
        xp += 50;
        printf("(received 50 experience points)'\n");
        health+=100;
        printf("(received 100 health)\n");
    }
    else if(currentCard->cardType == CARD_DEAD_END && !squirrelEventHasHappened ) //happens immediatley, no loop requirement
    {
        squirrelEventHasHappened = true;
        printf("\nA squirrel runs out of the forest...\n");
        printf("Jovi sees the squirrel and points toward it.\n");
        printf("The squirrel runs back into the forest and disappears.\n\n");
        xp += 5;
        printf("(received 5 experience points)\n");
    }
    else if(currentCard->cardType == CARD_ECHOING_HOLLOW && !echoEventHasHappened ) //happens immediatley, no loop requirement
    {
        echoEventHasHappened = true;
        printf("\nYou shout 'Hello!'\n");
        printf("You hear the echo.\n");
        printf("hello ... \n");
        printf("  ...  hello ... \n");
        printf("           ... hello ... \n\n");
        xp += 5;
        printf("(received 5 experience points)\n");
    }
    else if(currentCard->cardType == CARD_SECOND_PATH && !treeFallEventHasHappened && loops > 100)
    {
        treeFallEventHasHappened = true;
        printf("\nA tree falls and lands like a bridge across the river...\n");
        printf("There is now a path to cross the river.\n\n");
        //up the links by 1 and add the new transition
        deck[CARD_SECOND_PATH].numLinkedCards = 3;//was 2 so now 3
        deck[CARD_SECOND_PATH].linkedCards[2].cardType = CARD_OTHER_SIDE_RIVER; //add link to other side of river
        strcpy(deck[CARD_SECOND_PATH].linkedCards[2].name,"tree"); //link name
        //change description to fit the change
        strcpy(deck[CARD_SECOND_PATH].description, "You take the path deep into the woods.\nThe path continues to a garden.\nThere is also a deep river nearby.\nA tree has fallen across the river so you can cross to the other side.");
    }
    else if(currentCard->cardType == CARD_ELDERFERN_2 && !elderfernEventHasHappened ) //happens immediatley, no loop requirement
    {
        elderfernEventHasHappened = true;
        printf("\nThe mist of the forest is too thick, you cant see...\n");
        printf("You can hear the howl of wolves.\n\n");
    }
    else if(currentCard->cardType == CARD_GREY && books > 0 ) //happens immediatley, no loop requirement
    {
        printf("\nAlistair says 'Great, you brought me some books!'\n");
        for(int i = 0; i < books; i++)
        {
            printf("(received 1000 coins)\n");
            money+=1000;
            booksDeliveredToAlistair+=1;
        }
        printf("\n\n");
        books=0;
    }
    else if(currentCard->cardType == CARD_STACK_HOMESTEAD && !stackTreeFallEventHasHappened && xp > 250)
    {
        stackTreeFallEventHasHappened = true;
        printf("\nA tree falls and lands like a bridge across the river...\n");
        printf("There is now a path to cross the river.\n\n");
        //up the links by 1 and add the new transition
        deck[CARD_STACK_HOMESTEAD].numLinkedCards = 3;//was 2 so now 3
        deck[CARD_STACK_HOMESTEAD].linkedCards[2].cardType = CARD_STACK_RIVER; //add link to other side of river
        strcpy(deck[CARD_STACK_HOMESTEAD].linkedCards[2].name,"tree"); //link name
        //change description to fit the change
        strcpy(deck[CARD_STACK_HOMESTEAD].description, "You are at the Stack Homestead.\nThere is a mountain to the north.\nOn the far western side of the homestead, there is a river.\nA tree has fallen across the river so you can cross to the other side.\nThere is a pumpkin patch.\nThere is a fireplace here.");
    }
    else if(currentCard->cardType == CARD_FORSAKEN_ROAD && !chasmEventHasHappened && xp > 120 && deck[CARD_FORSAKEN_ROAD].numLinkedEnemies == 0 && loops > 600)
    {
        chasmEventHasHappened = true;
        printf("\nThe Blue Wizard appears ...\n");
        printf("\nAs the sky darkens and a swirling wind picks up,\nthe Blue Wizard steps forward,\nhis robes shimmering with arcane energy.\n");
        printf("With a wave of his staff, the ground trembles,\nand ethereal blue light pours from the staff's tip,\nflowing into the vast chasm that splits the road.\n");
        printf("Cracks in the earth glow as ancient magic mends the rift,\nstones rising from the depths and locking into place.");
        printf("Slowly, the chasm begins to close, the road restored.\n");
        printf("The air crackles with power as the wizard completes his work,\n");
        printf("leaving behind a road once more passable,\nbut now marked with the faint glow of magic.\n");
        printf("\nThe Blue Wizard leaves.\n");
        printf("The chasm is now repaired. You can continue north.\n\n");
        //up the links by 1 and add the new transition
        deck[CARD_FORSAKEN_ROAD].numLinkedCards = 2;//was 1 so now 2
        deck[CARD_FORSAKEN_ROAD].linkedCards[1].cardType = CARD_FORSAKEN_ROAD_2; //add link
        strcpy(deck[CARD_FORSAKEN_ROAD].linkedCards[1].name,"north"); //link name
        //change description to fit the change
        strcpy(deck[CARD_FORSAKEN_ROAD].description, "A desolate, overgrown path,\nonce traveled but now abandoned.\nCracked stone and twisted roots litter the way,\nand an eerie silence hangs in the air,\nas if the road itself has been forgotten by time.\nThe chasm is repaired. You can go north or south.");
    }
    else if(currentCard->cardType == CARD_EMBERFELL_CHAMBER && !ghostEventHasHappened) //happens immediatley, no loop requirement
    {
        ghostEventHasHappened = true;
        printf("\nA ghost appears. Its very spooky. Jovi barks at it.\n");
        printf("The ghost glides back into the wall and disappears.\n\n");
    }
    else if(currentCard->cardType == CARD_FIRST_ROAD_RIGHT && !jovi1EventHasHappened ) //happens immediatley, no loop requirement
    {
        jovi1EventHasHappened = true;
        printf("\nJovi barks...\n");
    }
    else if(currentCard->cardType == CARD_CAVE_DEEP && !jovi2EventHasHappened ) //happens immediatley, no loop requirement
    {
        jovi2EventHasHappened = true;
        printf("\nJovi barks...\n");
    }
    else if(currentCard->cardType == CARD_MIRKWOOD_ENTER && !jovi3EventHasHappened ) //happens immediatley, no loop requirement
    {
        jovi3EventHasHappened = true;
        printf("\nJovi barks...\n");
    }
    else if(currentCard->cardType == CARD_TOMBS && !jovi4EventHasHappened ) //happens immediatley, no loop requirement
    {
        jovi4EventHasHappened = true;
        printf("\nJovi barks...\n");
    }
    else if(currentCard->cardType == CARD_MARSH_11 && !jovi5EventHasHappened ) //happens immediatley, no loop requirement
    {
        jovi5EventHasHappened = true;
        printf("\nJovi barks...\n");
    }
    else if(currentCard->cardType == CARD_FARM_BAKER && !jovi6EventHasHappened ) //happens immediatley, no loop requirement
    {
        jovi6EventHasHappened = true;
        printf("\nJovi barks...\n");
    }
    else if(currentCard->cardType == CARD_PUB && !pub1EventHasHappened ) //happens immediatley, no loop requirement
    {
        pub1EventHasHappened = true;
        printf("\n'Hey there young fellow!'\n");
        printf("A drunk patron Walks over to you.\n");
        printf("'Whats your deal!' says the patron\n");
    }
    else if(currentCard->cardType == CARD_PUB && pub1EventHasHappened && !pub2EventHasHappened && hasSword) //no loop requirement, happens after event 1 and only if you have the sword
    {
        pub2EventHasHappened = true;
        printf("\nThe drunk patron punches you.\n");
        printf("You fall backward, but quickly get back up and draw your sword.\n");
        printf("'... I dont want no trouble' says the patron.\n");
        printf("He walks away.\n");
        xp += 50;
        printf("(received 50 experience points)\n");
    }
}

//handle the casting of spells
void handleCast(char* spell)
{
    if(mana <= 0){printf("no mana\n");return;}
    int manaToSubctract = 5;
    printf("( %d mana used ) \n", manaToSubctract);
    mana-=manaToSubctract;
    if (strcmp(spell, "fire") == 0)
    {
        printf("You cast fire...\n");
        if(currentCard->cardType == CARD_HOME)
        {
            printf("The fire place is lit.\n");
            homeFirePlaceIsLit = true;
        }
        else if(currentCard->cardType == CARD_OLD_MAN_JEFFERS)
        {
            printf("The fire place is lit at old man Jeffers.\n");
            jeffersFirePlaceIsLit = true;
        }
        else if(currentCard->cardType == CARD_CINDERSPIRE)
        {
            printf("The fire place is lit at Cinderspire.\n");
            cinderSpireFirePlaceIsLit = true;
        }
        else if(currentCard->cardType == CARD_OLD_MAN_MCCANN)
        {
            printf("The fire place is lit at old man McCann's.\n");
            mccannsFirePlaceIsLit = true;
        }
        else if(currentCard->cardType == CARD_WILLIAMS_TOWN_HALL)
        {
            printf("The fire place is lit at Williams Town Hall.\n");
            williamsFirePlaceIsLit = true;
        }
        else if(currentCard->cardType == CARD_STACK_HOMESTEAD)
        {
            printf("The fire place is lit at Stack Homestead.\n");
            stackFirePlaceIsLit = true;
        }
        else if(currentCard->cardType == CARD_VILLAGE)
        {
            printf("The fire place is lit at The Enchanted Village.\n");
            villageFirePlaceIsLit = true;
        }
        else if(currentCard->cardType == CARD_AJ)
        {
            printf("The fire place is lit at old man AJ's.\n");
            ajFirePlaceIsLit = true;
        }
        else if(currentCard->cardType == CARD_TOWN_HOUSE)
        {
            printf("The woman screams 'What are you doing, you are going to burn the place down!'.\n");
            printf("(received 5 xp)\n");
            xp += 5;
        }
        else if(currentCard->cardType == CARD_BELLEVILLE_LIBRARY)
        {
            printf("The fire place is lit at The Belleville Library.\n");
            bellevilleFirePlaceIsLit = true;
        }
        else
        {
            printf("no effect\n");
        }
    }
    else if (strcmp(spell, "grow") == 0)
    {
        printf("You cast grow...\n");
        if(currentCard->cardType == CARD_JEFFERS_GARDEN)
        {
            printf("One of the tomatoes grows incredibly large.\n");
            printf("You eat it.\n");
            printf("(received 800 health)\n");
            health += 800;
        }
        else if(currentCard->cardType == CARD_WILD_GARDEN)
        {
            printf("One of the apples grows incredibly large.\n");
            printf("You pick it from the tree and eat it.\n");
            printf("(received 500 health)\n");
            health += 500;
        }
        else if(currentCard->cardType == CARD_MCCANN_GARDEN)
        {
            printf("One of the tomatoes grows incredibly large.\n");
            printf("You eat it.\n");
            printf("(received 800 health)\n");
            health += 800;
        }
        else if(currentCard->cardType == CARD_STACK_HOMESTEAD)
        {
            printf("One of the pumpkins grows incredibly large.\n");
            printf("You eat it.\n");
            printf("(received 600 health)\n");
            health += 600;
        }
        else if(currentCard->cardType == CARD_EMBERFELL_GARDEN)
        {
            printf("One of the tomatoes grows incredibly large.\n");
            printf("You eat it.\n");
            printf("(received 800 health)\n");
            health += 800;
        }
        else if(currentCard->cardType == CARD_FARM_BAKER)
        {
            printf("One of the beets grows incredibly large.\n");
            printf("You eat it.\n");
            printf("(received 1200 health)\n");
            health += 1200;
        }
        else
        {
            printf("no effect\n");
        }
    }
    else if (strcmp(spell, "open") == 0)
    {
        if(currentCard->cardType == CARD_WILD_GARDEN)
        {
            printf("The locked shed door opens.\n");
            printf("You enter the gardening shed.\n");
            currentCard = &deck[CARD_FIRST_SHED];
        }
        else if (currentCard->cardType == CARD_IRONWOOD_CLEARING)
        {
            printf("The locked shed door opens.\n");
            printf("You enter the workman's shed.\n");
            currentCard = &deck[CARD_SECOND_SHED];
        }
        else if(currentCard->cardType == CARD_MCCANN_GARDEN)
        {
            printf("The locked shed door opens.\n");
            printf("You enter the McCann's gardening shed.\n");
            currentCard = &deck[CARD_THIRD_SHED];
        }
        else if(currentCard->cardType == CARD_WILLIAMS)
        {
            printf("The locked door to Town Hall opens.\n");
            printf("You enter the Williams Town Hall.\n");
            currentCard = &deck[CARD_WILLIAMS_TOWN_HALL];
        }
        else if(currentCard->cardType == CARD_EMBERFELL_CASTLE_ENTER)
        {
            printf("The locked door to Emberfell Castle opens.\n");
            printf("You enter The Grand Emberfell Hall.\n");
            currentCard = &deck[CARD_EMBERFELL_HALL];
        }
        else if(currentCard->cardType == CARD_EMBERFELL_TOWER && !emberfellTowerChestOpened)
        {
            emberfellTowerChestOpened = true;
            printf("The locked treasure chest opens.\n");
            printf("(received 1000 coins)\n");
            money+=1000;
        }
        else if(currentCard->cardType == CARD_CAVE_DEEP)
        {
            printf("The locked door to the Hidden Passage opens.\n");
            printf("You enter the Hidden Passage.\n");
            currentCard = &deck[CARD_HIDDEN_PASSAGE];
        }
        else if(currentCard->cardType == CARD_CAVERNS)
        {
            printf("The locked door to the Hidden Passage opens.\n");
            printf("You enter the Hidden Passage.\n");
            currentCard = &deck[CARD_HIDDEN_PASSAGE];
        }
        else
        {
            printf("no effect\n");
        }
    }
    else
    {
        printf("I dont know that spell.\n");
    }
}

//I ran into a situation where I sent a message to jovi, and the LLM response contained what looked
//like non-printable characters, and the game sort of froze, so to prevent that
//we now sanatize the response before printing each token
void removeNonPrintableChars(char *str) {
    int i = 0, j = 0;
    
    // Traverse through the string
    while (str[i]) {
        // If the character is printable, copy it to the new position
        if (isprint((unsigned char)str[i])) {
            str[j++] = str[i];//this code is kind of Genius, thanks to Chatgpt
        }
        i++;
    }
    
    // Null terminate the modified string
    str[j] = '\0';
}

// Function to process a stream of JSON objects
void process_json_stream(char *response) {
    char *start = response;
    char *end;
    cJSON *json_object;
    char *result = NULL;
    
    printf("\nJovi says: \n");
    // Loop through the response buffer and extract individual JSON objects
    while ((end = strchr(start, '\n')) != NULL) { // Assuming objects are separated by newlines
        *end = '\0'; // Null-terminate the current JSON object
        json_object = cJSON_Parse(start);  // Parse the current JSON object
        
        if (json_object) {
            // Successfully parsed a JSON object
            char *json_string = cJSON_PrintUnformatted(json_object);
            //printf("Received JSON object: %s\n", json_string);
            cJSON *response_item = cJSON_GetObjectItem(json_object, "response");
            //printf("response: %s\n", response_item->valuestring);
            removeNonPrintableChars(response_item->valuestring);//does this cause a memory leak? I checked with valgrind and it does not seem to
            printf("%s",response_item->valuestring);
            free(json_string);
            cJSON_Delete(json_object);
            
        } else {
            // Handle parse errors (if any)
            //printf("Error parsing JSON object.\n");
            printf("Bark bark ... bark ...\n");
        }

        // Move the pointer to the next JSON object (skip the newline character)
        start = end + 1;
    }
    //printf("\n%s\n", result);
    printf("\n\n");
}

//handle the networking calls of the jovi command
//big thanks to Chatgpt for helping me with the api call and the JSON stuff
void handleJovi(char* input) {
    CURL *curl;
    CURLcode res;
    char response[MAX_NET_RESP_LENGTH]; // Buffer to hold the response data

    char result[MAX_INPUT_UNIQUE_JOVI];  // Define a buffer to hold the fully qualified prompt
    // Safely format into the result buffer using snprintf
    snprintf(result, MAX_INPUT_UNIQUE_JOVI, "You are a talking dog named Jovi, I say '%s', how do you respond? (please keep answer to 3 sentences or less)", input);
    //printf("result - %s\n", result); //test the wrapped message
    // Prepare JSON data using cJSON
    cJSON *json_data = cJSON_CreateObject();
    cJSON_AddStringToObject(json_data, "model", "llama3"); // update model name per your setup
    cJSON_AddStringToObject(json_data, "prompt", result);

    // Convert the JSON object to a string
    char *json_string = cJSON_PrintUnformatted(json_data);

    // Initialize curl
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        // Set the URL for the POST request
        curl_easy_setopt(curl, CURLOPT_URL, "http://127.0.0.1:11434/api/generate");  // Update URL as per your setup

        // Set the HTTP request type to POST
        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        // Set the request body (JSON data)
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_string);

        // Set the Content-Type header to application/json
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Set the callback to handle the response
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

        // Perform the request
        res = curl_easy_perform(curl);

        // Check for errors
        if (res != CURLE_OK) {
            //fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            printf("Woof woof!\n");
        } else {
            // Process the stream of JSON objects
            process_json_stream(response);
        }

        // Clean up
        if(curl!=NULL){curl_easy_cleanup(curl);}
        curl_slist_free_all(headers);
    }

    // Clean up the JSON object
    cJSON_Delete(json_data);
    free(json_string);

    // Clean up curl global state
    curl_global_cleanup();
}
