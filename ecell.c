/* Elementary Cellular Automata Generator
 * by John Graham
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>


/* A container for the "population" at each step.  Each bit specifies the
 * state of an individual cell in the line.
 * So, when "POP == 12", the bits specify for a population that looks like
 * "0000 0000 0000 0000 0000 0000 0000 1100".
 */
unsigned int POP;

/* The rule for generating subsequen populations.  As each cell only
 * "looks" at the immediate left and right adjacent cells (with wrap
 * around), there are only 2^3 = 8 combinations.  As a result, each bit
 * represents the result of each combination for the next generation of the
 * central cell.
 *
 * As an example, "RULE == 16", which translates to "0001 0000", results in
 * the rule structure as follows:
 * 111 110 101 100 011 010 001 000
 *  0   0   0   1   0   0   0   0
 */
unsigned char RULE;

/* The number of generations to run through.  Simple enough.
 */
unsigned int NUM_GEN;

/* Output file pointer.
 */
FILE* OUTFILE;

/* Output file name (pointer to argument).
 */
char* OUTFILENAME;

/* Stores the arguments given and options taken.  Will take on the format
 * _ _ n o  v q p r
 * where "v" is verbose, "q" is quiet (can't both be 1, but can have both 0),
 * "o" is output to file, "p" is initial population (as in, it has been set),
 * "r" is rule (as in, it has been set), and n is the number of generations
 * (being set, as all else).
 */
unsigned char ARGUMENTS;


unsigned int applyrule(char view);
void nextgen();
void printhelp();
void printpop();
int readparam(char* parameter, unsigned int* container);


/* Applies the rule to the current perspective and returns either a 0 (dead) or
 * 1 (alive).
 */
unsigned int applyrule(char view) {
    
    /* This makes me feel pretty.  So, here we use the value of "view" to bit-
     * shift across the RULE to the value the rule would give for the next
     * generation.  The bit at 2^0 in RULE corresponds to the next generation's
     * value when "view == 0", same for the bit at 2^1 when "view == 1", and so
     * on for each bit "2^view".  Sexy.
     */
    if ((RULE & (1 << view)) == (1 << view))
        return 1;
    else
        return 0;
    
}


/* Uses the rule to generate the next generation, given the current population.
 * For the sake of simplicity, I'm assuming that limiting the number of
 * generations is handled by the calling (in our case, main) function.
 */
void nextgen() {
    
    unsigned int tmppop = 0;
    unsigned int newpop = 0;
    char view = 0;  // the view a cell has of its neighbors
    
    int i; // it's a counter!
    
    for (i = 31; i >= 0; i--) {
        
        /* Copy and bit-shift the sector of the population we're looking at so
         * that we have a concise view of the currently selected cell's
         * neighborhood.
         */
        if (i == 31) {
            
            // at the beginning (leftmost side), accounting for wrap-around
            tmppop = POP & 3221225473; // == POP & 1100 0000 ... 0000 0001
            view = (tmppop << 2) + (tmppop >> 30);
            
            // now, determine next value according to rule and apply to newpop
            newpop |= (applyrule(view) << 31);
            
        } else if (i == 0) {
            
            // at the end (rightmost side), accounting for wrap-around
            tmppop = POP & 2147483651; // == POP & 1000 0000 ... 0000 0011
            view = (tmppop << 1) + (tmppop >> 31);
            
            // now, determine next value according to rule and apply to newpop
            newpop |= applyrule(view);
            
        } else {
            
            // somewhere in the middle, can pattern this out
            tmppop = POP & (7 << (i - 1));  // == POP & ((0's)...111...(0's))
            view = tmppop >> (i - 1);
            
            // apply rule and add to newpop
            newpop |= (applyrule(view) << (i));
            
        }
        
    }
    
    POP = newpop;
    return;
}


/* Prints the input generation out to command line and, if OUTFILE is not null,
 * prints to the output file.
 */
void printpop() {
    
    int i;
    unsigned int j;
    
    // if not quiet mode
    if (ARGUMENTS % 8 <= 3) {
                
        for (i = 31; i >= 0; i--) {
            
            j = POP & (1 << i);
            
            if (j == (1 << i))
                printf("%s", "[]");
            else
                printf("%s", "__");
            
        }
        
        printf("\n");
    }
    
    // if output file given
    if (ARGUMENTS % 32 > 15)
        fprintf(OUTFILE, "%u\n", POP);
    
    return;
}


/* Prints the help message.  Nothing big or crazy.  Useful to have as a separate
 * function, because there's multiple reasons to call it when examining the
 * arguments and it's not the easiest to group them together.  Or, at least, I
 * think this works better than doing 5 calls to strcmp then defaulting.
 */
void printhelp() {
    
    printf("Usage: ecell [[OPTION] [PARAMETER]]\n");
    printf("Produces elementary cellular automata.\n\n");
    printf("Arguments that take parameters require said parameters.  Can be run without arguments, at which point any needed parameters are set randomly.\n");
    printf("\t-r\t[RULE]\t\tuse rule specified (8-bit unsigned int)\n");
    printf("\t-p\t[POPULATION]\tuse initial population (32-bit unsigned int)\n");
    printf("\t-q\t\t\t\"quiet mode\", no command line output\n");
    printf("\t-v\t\t\t\"verbose mode\", prints everything and the\n");
    printf("\t\t\t\t\tkitchen sink to the command line\n");
    printf("\t-o\t[OUTFILENAME]\tprints generated populations to output file.\n");
    printf("\t-h\t\t\tprints this help and exit\n\n");
    printf("If both -q and -v are entered, or arguments are improperly specified, or any other kind of slip-up, throws hands in the air, prints help, and quits.\n");
    
    return;
}


/* Reads the parameter, checks it's properly formatted, and puts it into the
 * container.  Returns 0 if all's good, -1 if something boned up.
 */
int readparam(char* parameter, unsigned int* container) {
    
    int i = 0;
    *container = 0;
    
    // while we still have an integer
    while ((parameter[i] >= '0') && (parameter[i] <= '9')) {
        
        // first, check that we aren't about to go out of bounds
        // if we are, return -1
        if (
            (*container > 429496729)
            || ((*container == 429496729) && (parameter[i] > '5'))
        ) return -1;
        
        *container = 10*(*container) + (parameter[i] - '0');
        i++;
    }
    
    if (i == 0)
        return -1;
    
    return 0;
}


int main (int argc, char* argv[]) {
    
    int i, j; // everyone needs a counter every now and then
    unsigned int param;  // parameter container
    
    /* Here, we read the input and operate on it.  If no input, we'll generate
     * a random population, random rule, no output file, and limit the number
     * of successive generations to 31 (so that we have a nice, pretty 32x32
     * square).  If the "-h" option is passed (or shitty input), print the
     * basic help documentation and exit.
     */
    ARGUMENTS = 0; // initiate the accepted arguments to 0
    for (i = 1; i < argc; i++) {
        /* Accepted arguments:
         * -r [RULE]        use rule specified (int between 0 and 255)
         * -p [POPULATION]  use this initial population (32-bit unsigned int)
         * -q               "quiet mode", no command line output
         * -v               "verbose mode", prints everything and the kitchen
         *                      sink to command line
         * -o [OUTFILENAME] prints generated populations to output file
         * -h               prints this help and exit
         * 
         * If both -q and -v are entered, or arguments are improperly
         * specified, or any other kind of slip-up, throws hands in the air,
         * prints help, and quits.
         */
        
        // identify and operate on input values
        if (argv[i][0] != '-') {
            
            // bad input, print help
            // when you've got problems, you know...
            printhelp();
            return 1;
            
        } else {
            switch (argv[i][1]) {
                case 'r':
                    // got ourselves a rule
                    
                    // first, check that we haven't already gotten one
                    if (ARGUMENTS % 2 > 0) {
                        printf("Too many rule invocations, here's some help for you to ponder.\n\n");
                        printhelp();
                        return 1;
                    }
                    ARGUMENTS += 1;
                    
                    // now take rule input
                    i++;
                    if (i >= argc) {
                        printf("Not enough arguments, here's some help for you to ponder.\n\n");
                        printhelp();
                        return 1;
                    }
                    j = readparam(argv[i], &param);
                    if (j == -1 || param > 255) {
                        // bad input, print help and exit
                        // have to check for less than 255, readparam only looks
                        //  for input inappropriate for an unsigned int
                        printf("Bad rule input given, here's some help for you to ponder.\n\n");
                        printhelp();
                        return 1;
                    }
                    // we've got a good parameter, put it into RULE and continue
                    RULE = param;
                    
                    break;
                
                case 'p':
                    // got an initial population
                    
                    // first, check that we haven't already gotten one
                    if (ARGUMENTS % 4 > 1) {
                        printf("Too many population invocations, here's some help for you to ponder.\n\n");
                        printhelp();
                        return 1;
                    }
                    ARGUMENTS += 2;
                    
                    // now take the population parameter
                    i++;
                    if (i >= argc) {
                        printf("Not enough arguments, here's some help for you to ponder.\n\n");
                        printhelp();
                        return 1;
                    }
                    j = readparam(argv[i], &param);
                    if (j == -1) {
                        // bad input, print help and exit
                        printf("Bad initial population input given, here's some help for you to ponder.\n\n");
                        printhelp();
                        return 1;
                    }
                    // we've got a good parameter, put it into POP and continue
                    POP = param;
                    
                    break;
                
                case 'q':
                    // quiet mode
                    
                    // first, check that we haven't already gotten this option
                    if (ARGUMENTS % 8 > 3) {
                        printf("Too many \"quiet mode\" invocations, here's some help for you to ponder.\n\n");
                        printhelp();
                        return 1;
                    }
                    ARGUMENTS += 4;
                    
                    // no parameters, continue scanning
                    
                    break;
                
                case 'v':
                    // verbose mode
                    
                    // first, check that we haven't already gotten this option
                    if (ARGUMENTS % 16 > 7 ) {
                        printf("Too many \"verbose mode\" invocations, here's some help for you to ponder.\n\n");
                        printhelp();
                        return 1;
                    }
                    ARGUMENTS += 8;
                    
                    // no parameters, continue scanning
                    
                    break;
                
                case 'o':
                    // print to output file
                    
                    // first, check that we haven't already gotten this option
                    if (ARGUMENTS % 32 > 15) {
                        printf("Too many output files specified, here's some help for you to ponder.\n\n");
                        printhelp();
                        return 1;
                    }
                    ARGUMENTS += 16;
                    
                    // attempt to open file
                    i++;
                    if (i >= argc) {
                        printf("Not enough arguments, here's some help for you to ponder.\n\n");
                        printhelp();
                        return 1;
                    }
                    OUTFILE = fopen(argv[i], "w");
                    if (OUTFILE == NULL) {
                        printf("Bad output file chosen, here's some help for you to ponder.\n\n");
                        printhelp();
                        return 1;
                    }
                    
                    // we've got an output file!
                    OUTFILENAME = argv[i];
                    
                    break;
                
                case 'n':
                    // number of generations to print
                    
                    // first, check that we haven't already gotten this option
                    if (ARGUMENTS % 64 > 31) {
                        printf("Too many numbers of generations to, well, generate, here's some help for you to ponder.\n\n");
                        printhelp();
                        return 1;
                    }
                    ARGUMENTS += 32;
                    
                    // now take the number of generations parameter
                    i++;
                    j = readparam(argv[i], &param);
                    if (j == -1) {
                        // bad input, print help and exit
                        printf("Bad number of generations to, well, generate given, here's some help for you to ponder.\n\n");
                        printhelp();
                        return 1;
                    }
                    // we've got a good parameter, put it into POP and continue
                    NUM_GEN = param;
                    
                    break;
                
                default:
                    // bad arguments (or "-h"), so print help
                    if (argv[i][1] != 'h')
                        printf("Bad arguments, here's some help for you to ponder.\n\n");
                    printhelp();
                    return 0;
            }
        }
    }
    
    // quick check to see that both quiet and verbose haven't been activated
    if (ARGUMENTS % 16 > 7 && ARGUMENTS % 8 > 3) {
        printf("Both quiet and verbose mode cannot be activated at the same time, here's some help for you to ponder.\n\n");
        printhelp();
        return 1;
    }

    // in case of no input, generate whatever we need randomly
    srand(time(NULL));
    
    // The maximum random value is 2^31 - 1, where we need up to 2^32 - 1,
    // so I'm assigning half the value of POP at a time.
    if (ARGUMENTS % 4 <= 1)
        POP = (rand() % 65535 << 16) + (rand() % 65535);
    if (ARGUMENTS % 2 <= 0)
        RULE = rand() % 255;
    if (ARGUMENTS % 64 <= 31)
        NUM_GEN = 31;
    
    // if in verbose mode
    if (ARGUMENTS % 16 > 7) {
        
        printf("Initial population = %u, rule = %u, number of generations = %u", POP, RULE, NUM_GEN);
        if (ARGUMENTS % 32 > 15)
            printf(", printing to %s.\n", OUTFILENAME);
        else
            printf(", no output file given.\n");
        
        // now going to print a visual representation of the rule
        printf("The rule %u corresponds to...\n", RULE);
        printf("111\t110\t101\t100\t011\t010\t001\t000\n");
        for (i = 7; i >= 0; i--)
            printf(" %u \t", (RULE & (1 << i)) >> i);
        
        printf("\n\n");
        printf("Generating...\n");
        
        // if we have an output file, print parameters
        if (ARGUMENTS % 32 > 15)
            fprintf(OUTFILE, "POP = %u, RULE = %u, NUM_GEN = %u\n", POP, RULE, NUM_GEN);
        
    }
    
    /* Now, generate succesive populations and print them out to command line
     * and output file as warranted.
     */
    printpop();
    for (i = 0; i < NUM_GEN; i++) {
        nextgen();
        printpop();
    }
    
    if (OUTFILE != NULL) fclose(OUTFILE);
    
    return 0;
}
