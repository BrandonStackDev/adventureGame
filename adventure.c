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
// Function to handle the response from the server, does not quite work, but good template
// size_t write_callback(void *ptr, size_t size, size_t nmemb, char *data) {
//     size_t total_size = size * nmemb;
//     if(data==NULL||ptr==NULL){return 0;}
//     strncat(data, ptr, total_size); // Append the response data to the response buffer
//     return total_size;
// }
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
        return total_size; 
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
const int MAX_INPUT_UNIQUE = 512;
const int MAX_INPUT_UNIQUE_JOVI = 4096;

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
Card deck[64];
int TotalNumberOfCards = 29;//increment deck size by powers of two, when this value is close to it

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
};

enum ItemTypes {
    ITEM_SWORD = 0,
    ITEM_MAP = 1,
    ITEM_HEALTH_POTION = 2,
    ITEM_MANA_POTION = 3,
    ITEM_COINS = 4,
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

//global state bools for fireplaces and casting
bool homeFirePlaceIsLit = false;
bool jeffersFirePlaceIsLit = false;
bool cinderSpireFirePlaceIsLit = false;

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
void process_json_stream(char *response);
void handleJovi(char* input);


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
                    //normalize input
                    formatInput(buffer);
                    //search for a match, if match set current card and break
                    for(int i = 0; i<currentCard->numLinkedCards;i++)
                    {
                        if(strcmp(buffer, currentCard->linkedCards[i].name) == 0)
                        {
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
                    bool allDead = true;
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
                            allDead = false;
                            break;
                        }
                        else
                        {
                            continue;
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
                if(jeffersFirePlaceIsLit){printf(" - jeffers\n");}
                if(cinderSpireFirePlaceIsLit){printf(" - spire\n");}
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
                        printf("went to old man jeffers.\n"); 
                        currentCard = &deck[CARD_OLD_MAN_JEFFERS];
                    }
                    else if (strcmp(buffer, "spire") == 0 && cinderSpireFirePlaceIsLit)
                    {
                        printf("went to Cinderspire.\n"); 
                        currentCard = &deck[CARD_CINDERSPIRE];
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
    deck[CARD_HOME].numLinkedItems = 3;
    strcpy(deck[CARD_HOME].name, "home");
    strcpy(deck[CARD_HOME].description, "You are in your home. Its dimly lit but cozy.\nThere is a fire place and a front door...");
    deck[CARD_HOME].linkedCards[0].cardType = CARD_FRONTDOOR; //link to front door
    strcpy(deck[CARD_HOME].linkedCards[0].name,"front"); //frontdoor link name
    deck[CARD_HOME].linkedCards[1].cardType = CARD_BACKDOOR; //link to back door / back yard
    strcpy(deck[CARD_HOME].linkedCards[1].name,"back"); //frontdoor link name
    deck[CARD_HOME].linkedItems[0] = ITEM_SWORD; //sword
    deck[CARD_HOME].linkedItems[1] = ITEM_HEALTH_POTION; //potion
    deck[CARD_HOME].linkedItems[2] = ITEM_MANA_POTION; //potion

    // Card 2  - outside the front door
    deck[CARD_FRONTDOOR].cardType = CARD_FRONTDOOR;
    deck[CARD_FRONTDOOR].numLinkedCards = 3;
    strcpy(deck[CARD_FRONTDOOR].name, "frontdoor");
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
    strcpy(deck[CARD_FIRST_ROAD_LEFT].name, "far down the road");
    strcpy(deck[CARD_FIRST_ROAD_LEFT].description, "You move left down the road until you come to a large tree.\nThe tree is very big and very old.");
    deck[CARD_FIRST_ROAD_LEFT].linkedCards[0].cardType = CARD_FRONTDOOR; //link to front door
    strcpy(deck[CARD_FIRST_ROAD_LEFT].linkedCards[0].name,"frontdoor"); //link name
    deck[CARD_FIRST_ROAD_LEFT].linkedCards[1].cardType = CARD_FIRST_CLIMB_TREE; //link to climable tree
    strcpy(deck[CARD_FIRST_ROAD_LEFT].linkedCards[1].name,"climb tree"); //link name

    // Card 4  - right on the road outside your house
    deck[CARD_FIRST_ROAD_RIGHT].cardType = CARD_FIRST_ROAD_RIGHT;
    deck[CARD_FIRST_ROAD_RIGHT].numLinkedCards = 2;
    strcpy(deck[CARD_FIRST_ROAD_RIGHT].name, "road toward town");
    strcpy(deck[CARD_FIRST_ROAD_RIGHT].description, "You move right down the road.\nLooks like trouble up ahead...");
    deck[CARD_FIRST_ROAD_RIGHT].linkedCards[0].cardType = CARD_FRONTDOOR; //link to front door
    strcpy(deck[CARD_FIRST_ROAD_RIGHT].linkedCards[0].name,"left"); //link name
    deck[CARD_FIRST_ROAD_RIGHT].linkedCards[1].cardType = CARD_FIRST_TROUBLE; //first trouble
    strcpy(deck[CARD_FIRST_ROAD_RIGHT].linkedCards[1].name,"trouble"); //link name

    // Card 5  - climb-able tree
    deck[CARD_FIRST_CLIMB_TREE].cardType = CARD_FIRST_CLIMB_TREE;
    deck[CARD_FIRST_CLIMB_TREE].numLinkedCards = 2;
    deck[CARD_FIRST_CLIMB_TREE].numLinkedItems = 2;
    strcpy(deck[CARD_FIRST_CLIMB_TREE].name, "up in the tree");
    strcpy(deck[CARD_FIRST_CLIMB_TREE].description, "You climb the tree.\nYou can see everything from up here.");
    deck[CARD_FIRST_CLIMB_TREE].linkedCards[0].cardType = CARD_DEAD_END; //link to first left road path
    strcpy(deck[CARD_FIRST_CLIMB_TREE].linkedCards[0].name,"dead"); //link name
    deck[CARD_FIRST_CLIMB_TREE].linkedCards[1].cardType = CARD_FIRST_ROAD_LEFT; //link to first left road path
    strcpy(deck[CARD_FIRST_CLIMB_TREE].linkedCards[1].name,"down"); //link name
    deck[CARD_FIRST_CLIMB_TREE].linkedItems[0] = ITEM_MAP; //map
    deck[CARD_FIRST_CLIMB_TREE].linkedItems[1] = ITEM_COINS; //some coins

    // Card 6  - further down the road to the right
    deck[CARD_FIRST_TROUBLE].cardType = CARD_FIRST_TROUBLE;
    deck[CARD_FIRST_TROUBLE].numLinkedCards = 2;
    deck[CARD_FIRST_TROUBLE].numLinkedEnemies = 2;
    strcpy(deck[CARD_FIRST_TROUBLE].name, "clearing along road");
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
    deck[CARD_DEAD_END].numLinkedCards = 2;
    strcpy(deck[CARD_DEAD_END].name, "dead end");
    strcpy(deck[CARD_DEAD_END].description, "You have reached a dead end after the tree.\nBut there is a path...");
    deck[CARD_DEAD_END].linkedCards[0].cardType = CARD_FIRST_PATH; //link to first path
    strcpy(deck[CARD_DEAD_END].linkedCards[0].name,"path"); //link name
    deck[CARD_DEAD_END].linkedCards[1].cardType = CARD_FIRST_CLIMB_TREE; //back to tree
    strcpy(deck[CARD_DEAD_END].linkedCards[1].name,"tree"); //link name

    // Card 8  - path to old man jeffers
    deck[CARD_FIRST_PATH].cardType = CARD_FIRST_PATH;
    deck[CARD_FIRST_PATH].numLinkedCards = 3;
    strcpy(deck[CARD_FIRST_PATH].name, "path in the woods");
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
    strcpy(deck[CARD_ROAD_AFTER_TROUBLE].name, "long stretch of road");
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
    strcpy(deck[CARD_OLD_MAN_JEFFERS].name, "old man jeffers");
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
    strcpy(deck[CARD_JEFFERS_GARDEN].name, "jeffers garden");
    strcpy(deck[CARD_JEFFERS_GARDEN].description, "You are in old man Jeffers garden.\nThere is a tomoato plant.");
    deck[CARD_JEFFERS_GARDEN].linkedCards[0].cardType = CARD_OLD_MAN_JEFFERS; //link to first path
    strcpy(deck[CARD_JEFFERS_GARDEN].linkedCards[0].name,"jeffers"); //link name
    deck[CARD_JEFFERS_GARDEN].linkedItems[0] = ITEM_HEALTH_POTION; //health
    deck[CARD_JEFFERS_GARDEN].linkedItems[1] = ITEM_HEALTH_POTION; //health

    // Card 12  - town square - has event
    deck[CARD_TOWN_SQUARE].cardType = CARD_TOWN_SQUARE;
    deck[CARD_TOWN_SQUARE].numLinkedCards = 5;
    deck[CARD_TOWN_SQUARE].numLinkedItems = 6;
    strcpy(deck[CARD_TOWN_SQUARE].name, "town square");
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
    strcpy(deck[CARD_MIRKWOOD_ENTER].description, "You take a small path to the entrance of Mirkwood forest.\nAll the trees seem to be dead.");
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
    strcpy(deck[CARD_CAVE_DEEP].description, "You are deep inside a cave.");
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
    strcpy(deck[CARD_TOWN_HOUSE].name, "Small House");
    strcpy(deck[CARD_TOWN_HOUSE].description, "You are in a small house.\nThere is a woman here.");
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
    strcpy(deck[CARD_SECOND_PATH].description, "You take the path deep into the woods.\nThe path continues.");
    deck[CARD_SECOND_PATH].linkedCards[0].cardType = CARD_BACKDOOR; //link back to backyard/backdoor
    strcpy(deck[CARD_SECOND_PATH].linkedCards[0].name,"back"); //link name
    deck[CARD_SECOND_PATH].linkedCards[1].cardType = CARD_WILD_GARDEN; //link to wild garden
    strcpy(deck[CARD_SECOND_PATH].linkedCards[1].name,"garden"); //link name

    // Card 25  - wild garden - has event, has grow
    deck[CARD_WILD_GARDEN].cardType = CARD_WILD_GARDEN;
    deck[CARD_WILD_GARDEN].numLinkedCards = 1;
    strcpy(deck[CARD_WILD_GARDEN].name, "Wild Garden");
    strcpy(deck[CARD_WILD_GARDEN].description, "You are in a wild garden with lots of plants to choose from.\nThere is an apple tree here.");
    deck[CARD_WILD_GARDEN].linkedCards[0].cardType = CARD_SECOND_PATH; //link to second path
    strcpy(deck[CARD_WILD_GARDEN].linkedCards[0].name,"path"); //link name

    // Card 26  - Bramblethorn Overlook
    deck[CARD_BRAMBLETHORN_OVERLOOK].cardType = CARD_BRAMBLETHORN_OVERLOOK;
    deck[CARD_BRAMBLETHORN_OVERLOOK].numLinkedCards = 1;
    strcpy(deck[CARD_BRAMBLETHORN_OVERLOOK].name, "Bramblethorn Overlook");
    strcpy(deck[CARD_BRAMBLETHORN_OVERLOOK].description, "The view is quite impressive.\nYou can see out over much of the forest and a river.\nThere is a bridge nearby.");
    deck[CARD_BRAMBLETHORN_OVERLOOK].linkedCards[0].cardType = CARD_MIRKWOOD_EXIT; //link back to mirkwood exit
    strcpy(deck[CARD_BRAMBLETHORN_OVERLOOK].linkedCards[0].name,"wood"); //link name
    //todo: bridge
    
    // Card 27  - Ironwood Clearing
    deck[CARD_IRONWOOD_CLEARING].cardType = CARD_IRONWOOD_CLEARING;
    deck[CARD_IRONWOOD_CLEARING].numLinkedCards = 1;
    deck[CARD_IRONWOOD_CLEARING].numLinkedEnemies = 2;
    strcpy(deck[CARD_IRONWOOD_CLEARING].name, "Ironwood Clearing");
    strcpy(deck[CARD_IRONWOOD_CLEARING].description, "The road opens up into a rare, serene clearing.\nThere is a lot of space.\nThe air is heavy with a quiet power, a feeling that this place is not entirely of the natural world.");
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


    // Set the starting room
    currentCard = &deck[0];
}


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
        mana += 200;
        break;

    case ITEM_COINS:
        printf("Found some coins\n");
        money += 200;
        break;

    default:
        printf("Did not recognize item type\n");
        break;
    }
}

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
        printf("you died...\n");//death in this game is like grand theft auto sort of...
        // Set the starting room back to home
        currentCard = &deck[0];
        health = 100;//set starting health to 100, you will have to gain a surplus back in most cases to continue
    }
}

int getDamage()
{
    if(xp > 1000)
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

void handleEvents()
{
    if(currentCard->cardType == CARD_HOME && !ringEventHasHappened && loops > 10)
    {
        ringEventHasHappened = true;
        printf("\nThere is a knock at the door...\n");
        printf("You open the door and standing there is a blue wizard.\n");
        printf("He hands you a magic ring, and then he leaves.\n\n");
        printf("You got the ring\n");
        hasRing = true;
    }
    if(currentCard->cardType == CARD_TOWN_SQUARE && !kingEventHasHappened && loops > 50)
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
            xp += 100;
            printf("(received 100 experience points)'\n");
        }
        printf("The king leaves and the crowd disbands.\n\n");
    }
    if(currentCard->cardType == CARD_WILD_GARDEN && !tomBomEventHasHappened && loops > 25)
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
        printf("(received 100 health)'\n");
    }
    if(currentCard->cardType == CARD_DEAD_END && !squirrelEventHasHappened ) //happens immedialtey, no loop requirement
    {
        squirrelEventHasHappened = true;
        printf("\nA squirrel runs out of the forest...\n");
        printf("Jovi sees the squirrel and points toward it.\n");
        printf("The squirrel runs back into the forest and disappears.\n\n");
        xp += 5;
        printf("(received 5 experience points)\n");
    }
    if(currentCard->cardType == CARD_ECHOING_HOLLOW && !echoEventHasHappened ) //happens immedialtey, no loop requirement
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
}

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
        else if(currentCard->cardType == CARD_TOWN_HOUSE)
        {
            printf("The woman's breasts grow larger.\n");
            printf("(received 50 xp)\n");
            xp += 50;
        }
        else if(currentCard->cardType == CARD_WILD_GARDEN)
        {
            printf("One of the apples grows incredibly large.\n");
            printf("You pick it from the tree and eat it.\n");
            printf("(received 500 health)\n");
            health += 500;
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
    printf("\n\n", result);
}

void handleJovi(char* input) {
    CURL *curl;
    CURLcode res;
    char response[MAX_NET_RESP_LENGTH]; // = "";  // Buffer to hold the response data

    char result[MAX_INPUT_UNIQUE_JOVI];  // Define a buffer
    // Safely format into the result buffer using snprintf
    snprintf(result, MAX_INPUT_UNIQUE, "You are a talking dog named Jovi, I say '%s', how do you respond? (please keep answer to 3 sentences or less)", input);

    // Prepare JSON data using cJSON
    cJSON *json_data = cJSON_CreateObject();
    cJSON_AddStringToObject(json_data, "model", "llama3");
    cJSON_AddStringToObject(json_data, "prompt", result);

    // Convert the JSON object to a string
    char *json_string = cJSON_PrintUnformatted(json_data);

    // Initialize curl
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        // Set the URL for the POST request
        curl_easy_setopt(curl, CURLOPT_URL, "http://10.0.0.253:11434/api/generate");  // Update URL as per your setup

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
