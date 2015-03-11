#define NUM_POLLS 7

struct poll_data {

  char *title;
  char *options[20];
  int votes[20];
  sbyte revote;
  sbyte active;

};

