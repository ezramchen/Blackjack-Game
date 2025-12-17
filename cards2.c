#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>

#define CSI "\x1b["

static inline void goto_xy(int r,int c){ printf(CSI "%d;%dH", r, c); }
static inline void clear_eol(void){ fputs(CSI "K", stdout); }       // erase to end of line
static inline void clear_line(void){ fputs(CSI "2K\r", stdout); }   // erase whole line
static inline void move_up(int n){ printf(CSI "%dA", n); }
static inline void move_down(int n){ printf(CSI "%dB", n); }

static long int balance = 0; // global variable for balance
static int draw; // global variable for draw count

// Function to swap two integers
static inline void swap(int *px, int *py) {
  int temp = *px;
  *px = *py;
  *py = temp;
}

// Function to shuffle an array of integers
static void shuffle(int *array, int n) {
  srand(time(0));
  for (int i = n - 1; i > 0; i--) {
    int j = rand() % (i + 1);
    swap(&array[i], &array[j]);
  }
}

// Function to create a deck of cards
static void deck(char *cna, char *cva, char *csa, int *cid) {
  for (int i = 0; i < 52; i++) {
    int j = 0, k = 0;
    while (*(cva + 10 * ((*(cid + i)) % 13) + j) != '\0') {
      *(cna + 20 * i + k) = *(cva + 10 * ((*(cid + i)) % 13) + j);
      j++, k++;
    } // copy card value to card name
    j = 0;
    while (*(csa + 10 * ((*(cid + i)) / 13) + j) != '\0') {
      *(cna + 20 * i + k) = *(csa + 10 * ((*(cid + i)) / 13) + j);
      j++, k++;
    } // copy card suit to card name
    *(cna + 20 * i + k) = '\0';
  }
  FILE *CheckCards = fopen("cards2.txt", "w");
  for (int i = 0; i < 52; i++) {
    fprintf(CheckCards, "%s %d\n", cna + 20 * i, *(cid + i) + 1);
  } // write card names and ids to file
  fclose(CheckCards);
}

// royal rule
static int sat(int val) {
  return (val & ((val - 10) >> 31)) | (10 & ~((val - 10) >> 31));
}

// hand value
static void drawing(int *hs, int *aces, int *cid, char *harr, char *cna) {
  int rank = (*(cid + draw) % 13);
  *hs += sat(rank + 1); // add card value to hand sum
  if (rank == 0) {
    (*aces)++; // increment ace count
  }
  for (int i = 0; i < 5; i++) {
    if (*(harr + 20 * i) == '\0') {
      memcpy(harr + 20 * i, cna + 20 * draw, 20);
      break; // copy card name to hand array
    }
  }
  draw++; // increment draw count
}

// user input
static inline long int uip(char tag) {
  char line[32];
  if (tag == 'p') {
    printf("BlACKJACK!\nDealer stands on 17\n\nWould you like to play a game?\nYes/No: ");
    fflush(stdout);
  }
  if (tag == 'b') {
    printf("\nHow much would you like to bet?\n");
  }
  if (tag == 'h') {
    printf("\nWould you like to hit or stand?\n");
  }
  if (tag == 'a') {
    printf("\nWould you like to play again?\nYes/No: ");
    fflush(stdout);
  }
  for (;;) {
    if (!fgets(line, sizeof line, stdin)) return EOF;
    if (!strchr(line, '\n')) {
      int ch; while ((ch = getc(stdin)) != '\n' && ch != EOF) {}
    }
    char *p = line + strspn(line, " \t");
    int c = tolower((unsigned char)*p);
    if ((c == 'y' || c == 'n') && (tag == 'p' || tag == 'a')) return c;
    if ((tag == 'h') && (c == 'h' || c == 's')) return c;
    if (tag == 'b') {
      errno = 0; long int v = strtol(p, NULL, 10); 
      if (v > 0 && errno != ERANGE) return v;
      printf("Please enter a valid bet.\n");
      continue;
    }
    printf("Please enter a valid input.\n");
  }
}

static inline void clear() {
  fputs("\x1b[3J\x1b[H\x1b[2J", stdout);
  fflush(stdout);
}

static inline void clear_block_down(int rows){
  for (int i=0;i<rows;i++){
    clear_line();
    if (i < rows-1) move_down(1);
  }
  if (rows > 1) move_up(rows-1); // return to starting row
}

// game logic
static int gameplay(int bet, int *cid, char *cna) {

  // initialize player and dealer hands
  int ps = 0, ds = 0, pa = 0, da = 0;
  char ph[5][20] = {{0}}, dh[5][20] = {{0}}, urp;

  // draw two cards for the player and dealer
  drawing(&ps, &pa, cid, &ph[0][0], cna);
  drawing(&ds, &da, cid, &dh[0][0], cna);
  drawing(&ps, &pa, cid, &ph[0][0], cna);
  drawing(&ds, &da, cid, &dh[0][0], cna);

  // hit/stand loop
  while (ps < 21 && ph[4][0] == 0) {

    // check for blackjack
    if (ps == 11 && pa == 1) {
      ps = 21;
      break;
    }

    // print player and dealer hands
    printf("Your hand:\n");
    for (int i = 0; i < 5; i++) {
      if (ph[i][0] != 0) {
        printf("%s\n", ph[i]);
      }
    }
    printf("Your total: %d\n", ps);
    printf("\nThe Dealer's card: %s\n", dh[1]);

    // user input
    urp = uip('h');
    if (urp == 's') {

      // clear screen
      move_up(9);
      clear_block_down(9);
      move_up(0);

      //stand
      break;
    }
    if (urp == 'h') {

      // clear screen
      move_up(9);
      clear_block_down(9);

      // hit
      drawing(&ps, &pa, cid, &ph[0][0], cna);
    }
  }

  // ace rule
  if (ps < 12 && pa == 1) ps += 10;

  // visual balancing
  if (ps >= 21) {
    move_up(2);
    clear_block_down(2);
    move_up(0);
  }

  // final tally
  printf("\nYour final hand:\n");
  for (int i = 0; i < 5; i++) {
    if (ph[i][0] != 0) {
      printf("%s\n", ph[i]);
    }
  }
  printf("Your total: %d\n", ps);

  // bust or blackjack
  if (ps > 21) {
    printf("\nYou busted! Womp Womp!\n");
  }
  if (ps == 21) {
    printf("\nBlackjack! You Win!\n");
    balance += bet * 2;
  }

  // dealer's turn
  if (ps < 21) {

    // dealer draws
    while (ds < 17 && dh[4][0] == 0) {
      if (ds > 5 && ds < 12 && da == 1) break;
      drawing(&ds, &da, cid, &dh[0][0], cna);
    }
    if (ds < 12 && da == 1) ds += 10;
    printf("\nThe Dealer's hand:\n");
    for (int i = 0; i < 5; i++) {
      if (dh[i][0] != 0) {
        printf("%s\n", dh[i]);
      }
    }
    printf("The Dealer's total: %d\n", ds);

    // winning conditions
    if (ds > 21) {
      printf("\nThe Dealer busted! You Win!\n");
      balance += bet * 2;
    }
    if (ds == 21) {
      printf("\nThe Dealer got Blackjack! You Lose!\n");
    }
    if (ds < 21) {
      if (ds > ps) {
        printf("\nThe Dealer wins! You Lose!\n");
      }
      if (ds < ps) {
        printf("\nYou win! The Dealer Lost!\n");
        balance += bet * 2;
      }
      if (ds == ps) {
        clear();
        printf("\nIt's a tie! PUSH!\n");
        gameplay(bet, cid, cna);
      }
    }
  }

  // back to letsplay
  return 1;
}

// launch game
static int letsplay(int *cid, char *cna, char *cva, char *csa) {

  // shuffle the deck
  shuffle(cid, 52);

  // fills the deck with the shuffled cards
  deck(cna, cva, csa, cid);

  // initializes draw count
  draw = 0;

  // wager
  long int bet = uip('b');
  balance -= bet;
  printf("\nYou have bet $%ld\n\n", bet);

  // gameplay
  gameplay(bet, cid, cna);

  // balance
  printf("\nYour balance is $%ld\n", balance);

  // play again
  if (uip('a') == 'y') {
    clear();
    letsplay(cid, cna, cva, csa);
  }
  return 0;
}

// main function
int main() {

  // array that represents all cards in a deck
  int cid[52];

  // fill the array with integers from 0 to 51
  for (int i = 0; i < 52; i++) {
    cid[i] = i;
  }

  // array that represents all card suits
  char csa[4][10] = {"Diamonds", "Clubs", "Hearts", "Spades"};

  // array that represents all card values
  char cva[13][10] = {"Ace of ", "Two of ", "Three of ", "Four of ", "Five of ", "Six of ", "Seven of ", "Eight of ", "Nine of ", "Ten of ", "Jack of ", "Queen of ", "King of "};

  // array that represents all card names
  char cna[52][20];

  // let's play
  if(uip('p') == 'y') {
    letsplay(&cid[0], &cna[0][0], &cva[0][0], &csa[0][0]);
  }

  // exit
  return 0;

}