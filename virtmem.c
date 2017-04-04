/*
 * Skeleton code for CSC 360, Spring 2017,  Assignment #3.
 *
 * Prepared by: Michael Zastre (University of Victoria) 2017
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define LOG(var) fprintf(stderr,"[LOG] %s:%d  :: %s\n",__func__,__LINE__, var);
//#define LOG(var) ;

/*
 * Some compile-time constants.
 */

#define REPLACE_NONE 0
#define REPLACE_FIFO 1
#define REPLACE_LRU  2
#define REPLACE_CLOCK 3
#define REPLACE_OPTIMAL 4


#define TRUE 1
#define FALSE 0
#define PROGRESS_BAR_WIDTH 60
#define MAX_LINE_LEN 100

#define MAX_INT_64 9223372036854775807


/*
 * Some function prototypes to keep the compiler happy.
 */
int setup(void);
int teardown(void);
int output_report(void);
long resolve_address(long, int);
void error_resolve_address(long, int);

long FIFO(long);
long LRU(long);
long CLOCK(long);

/* Amanda stuff */
void print(const char*);
long update_table(int frame, long page, long offset, int* swap);
long debug_last_mod = 0;



/*
 * Variables used to keep track of the number of memory-system events
 * that are simulated.
 */
int page_faults = 0;  /* Page does not exist in page table*/
int mem_refs    = 0;
int swap_outs   = 0;  /* Replace an existing page entry with your own*/
int swap_ins    = 0;  /* Place a page entry in a free slot */

/*position of the 'oldest' item in page_table, used for FIFO */
int oldest = 0;
long curr_time = 0;
long clock_hand = 0;

/*
 * Page-table information. You may want to modify this in order to
 * implement schemes such as CLOCK. However, you are not required to
 * do so.
 */
struct page_table_entry *page_table = NULL;
struct page_table_entry {
    long page_num;
    int dirty;
    int modified;
    int free;
    long time;
};


/*
 * These global variables will be set in the main() function. The default
 * values here are non-sensical, but it is safer to zero out a variable
 * rather than trust to random data that might be stored in it -- this
 * helps with debugging (i.e., eliminates a possible source of randomness
 * in misbehaving programs).
 */

int size_of_frame = 0;  /* power of 2 */
int size_of_memory = 0; /* number of frames */
int page_replacement_scheme = REPLACE_NONE;


/*
 * Function to convert a logical address into its corresponding
 * physical address. The value returned by this function is the
 * physical address (or -1 if no physical address can exist for
 * the logical address given the current page-allocation state.
 */

long resolve_address(long logical, int memwrite)
{
    char outbuff[200];

    int i;
    long page, frame;
    long offset;
    long mask = 0;
    long effective;

    /* Get the page and offset */
    page = (logical >> size_of_frame);

    sprintf(outbuff,"Program address 0x%08lx : VPN = 0x%08lx (using framesize = %d)",logical,page,size_of_frame);
    LOG(outbuff);

    for (i=0; i<size_of_frame; i++) {
        mask = mask << 1;
        mask |= 1;
    }
    offset = logical & mask;

    sprintf(outbuff,"Apply 0x%08lx to 0x%08lx to get offset = 0x%08lx", mask, logical, offset);
    LOG(outbuff);

    /* Find page in the inverted page table. */
    LOG("Does the VM have the page?");
    frame = -1;
    for ( i = 0; i < size_of_memory; i++ ) {
        if (!page_table[i].free && page_table[i].page_num == page) {
            frame = i;
            break;
        }
    }

    /* If frame is not -1, then we can successfully resolve the
     * address and return the result. */
    if (frame != -1) {
        sprintf(outbuff, "--> Yes! Best case scenario, page is in the page_table @ frame=%ld", frame);
        LOG(outbuff);

        effective = (frame << size_of_frame) | offset;
        page_table[frame].time = curr_time;
        page_table[frame].dirty = 1;
        if(page_replacement_scheme == REPLACE_CLOCK | page_replacement_scheme == REPLACE_LRU) {
         swap_ins++;
        }

        sprintf(outbuff,"==> Physical address for page 0x%08lx = 0x%ld:%ld = 0x%08lx", page, frame, offset, effective);
        LOG(outbuff);
        return effective;
    }

    /* If we reach this point, there was a page fault. Find
     * a free frame. */
    LOG("Damn it, page fault ... any free frames?");
    page_faults++;


    for ( i = 0; i < size_of_memory; i++) {
        if (page_table[i].free) {
            frame = i;
            break;
        }
    }


    /* If we found a free frame, then patch up the
     * page table entry and compute the effective
     * address. Otherwise return -1.
     */
    if (frame != -1) {
        //don't increment swap_ins on first fault
        sprintf(outbuff, "--> Yes! Found a free frame, page is in the page_table @ frame=%ld", frame);
        LOG(outbuff);

        page_table[frame].page_num = page;
        page_table[i].free = FALSE;
        page_table[frame].time = curr_time;
        page_table[frame].dirty = 1;
        effective = (frame << size_of_frame) | offset;

        sprintf(outbuff,"==> Physical address for page 0x%08lx = 0x%ld:%ld = 0x%08lx", page, frame, offset, effective);
        LOG(outbuff);
        return effective;
    } else {

        LOG("Ugh !?# There's nothing free, let's find a victim.");
        LOG("Here is where you need to apply your page replacement scheme.");

        swap_outs++;  //Have to replace an existing page aka, swap_out.

        switch (page_replacement_scheme) {
          case REPLACE_FIFO:
            frame = FIFO(page);
            swap_ins++;
            break;

          case REPLACE_LRU:
            swap_ins++;
            frame = LRU(page);
            break;

          case REPLACE_CLOCK:
            frame = CLOCK(page);
            break;
              
          default:
            return -1;
        }
        return((frame << size_of_frame) | offset);
    }
}

long FIFO(long page) {
  long frame = oldest;

  page_table[oldest].page_num = page; //Replace oldest value in page_table with new page.
  oldest = (oldest + 1)%size_of_memory; //Oldest is now next spot in the page_table, update it.
  return(frame);
}

long LRU(long page) {
  long lru_frame = 0;
  long min_time = MAX_INT_64;
  int i;

  for ( i = 0; i < size_of_memory; i++) { //Loop till find frame with lowest time.
      if (page_table[i].time < min_time) {
          lru_frame = i;
          min_time = page_table[i].time;
      }
  }

  page_table[lru_frame].page_num = page; //Replace least used value in page_table with new page.
  page_table[lru_frame].time = curr_time; //Set time of new page to the current time.

  return(lru_frame);

}

long CLOCK(long page) {

  while(TRUE) { //Keep going till you find a 0 dirty bit.
    if(page_table[clock_hand].dirty == 1) {
      page_table[clock_hand].dirty = 0; //Set all 1's to 0's
      clock_hand = (clock_hand + 1)%size_of_memory; //Increment clock_hand, mod to wrap around
      swap_ins++;
    } else { break; } //Found 0 dirty bit
  }

  page_table[clock_hand].page_num = page; //Insert new page
  page_table[clock_hand].dirty = 1;

  return(clock_hand);

}

void print(const char* instruction) {
    int frame;
    long page_num;
    int dirty;
    int modified;
    int free;

    fprintf(stderr, "-------------------------------------------------------------\n");
    fprintf(stderr, "%s   mem_refs: %d  page_faults: %d  swap_ins: %d  swap_outs: %d\n\n",
            instruction,mem_refs, page_faults, swap_ins, swap_outs);
    fprintf(stderr, "FRAME\t|PAGE\t\t|MOD\t|DIRTY\t\n");
    fprintf(stderr, "-------------------------------------------------------------\n");
    for ( frame = 0; frame < size_of_memory; frame++) {
        free = page_table[frame].free;
        modified = page_table[frame].modified;
        page_num = page_table[frame].page_num;
        dirty = page_table[frame].dirty;
        char* updated = "";

        if(debug_last_mod == frame)
            updated = "<-";

        if(free)
            fprintf(stderr, "%d\t|FREE\t\t|%d\t|%d\t%s\n", frame, modified, dirty,updated);
        else
            fprintf(stderr," %d\t|0x%lx\t|%d\t|%d\t%s\n", frame, page_num, modified, dirty,updated);
    }
    fprintf(stderr, "-------------------------------------------------------------\n");
    fflush(stderr);
}

/* End AMANDA */

/*
 * Super-simple progress bar.
 */
void display_progress(int percent)
{
    int to_date = PROGRESS_BAR_WIDTH * percent / 100;
    static int last_to_date = 0;
    int i;

    if (last_to_date < to_date) {
        last_to_date = to_date;
    } else {
        return;
    }

    printf("Progress [");
    for (i=0; i<to_date; i++) {
        printf(".");
    }
    for (; i<PROGRESS_BAR_WIDTH; i++) {
        printf(" ");
    }
    printf("] %3d%%", percent);
    printf("\r");
    fflush(stdout);
}


int setup()
{
    int i;

    LOG("Setting up page table (which is the page_table_entry struct) ...");

    page_table = (struct page_table_entry *)malloc(
        sizeof(struct page_table_entry) * size_of_memory
    );

    if (page_table == NULL) {
        fprintf(stderr,
            "Simulator error: cannot allocate memory for page table.\n");
        exit(1);
    }

    LOG("Labelling every page entry as FREE ...");

    for (i=0; i<size_of_memory; i++) {
        page_table[i].free = TRUE;
    }

    return -1;
}


int teardown()
{

    return -1;
}


void error_resolve_address(long a, int l)
{
    fprintf(stderr, "\n");
    fprintf(stderr,
        "Simulator error: cannot resolve address 0x%lx at line %d\n",
        a, l
    );
    exit(1);
}


int output_report()
{
    printf("\n");
    printf("Memory references: %d\n", mem_refs);
    printf("Page faults: %d\n", page_faults);
    printf("Swap ins: %d\n", swap_ins);
    printf("Swap outs: %d\n", swap_outs);

    return -1;
}


int main(int argc, char **argv)
{
    /* For working with command-line arguments. */
    int i;
    char *s;

    /* For working with input file. */
    FILE *infile = NULL;
    char *infile_name = NULL;
    struct stat infile_stat;
    int  line_num = 0;
    int infile_size = 0;

    /* For processing each individual line in the input file. */
    char buffer[MAX_LINE_LEN];
    long addr;
    char addr_type;
    int  is_write;

    /* For making visible the work being done by the simulator. */
    int show_progress = FALSE;

    /* Process the command-line parameters. Note that the
     * REPLACE_OPTIMAL scheme is not required for A#3.
     */
    for (i=1; i < argc; i++) {
        if (strncmp(argv[i], "--replace=", 9) == 0) {
            s = strstr(argv[i], "=") + 1;
            if (strcmp(s, "fifo") == 0) {
                page_replacement_scheme = REPLACE_FIFO;
            } else if (strcmp(s, "lru") == 0) {
                page_replacement_scheme = REPLACE_LRU;
            } else if (strcmp(s, "clock") == 0) {
                page_replacement_scheme = REPLACE_CLOCK;
            } else if (strcmp(s, "optimal") == 0) {
                page_replacement_scheme = REPLACE_OPTIMAL;
            } else {
                page_replacement_scheme = REPLACE_NONE;
            }
        } else if (strncmp(argv[i], "--file=", 7) == 0) {
            infile_name = strstr(argv[i], "=") + 1;
        } else if (strncmp(argv[i], "--framesize=", 12) == 0) {
            s = strstr(argv[i], "=") + 1;
            size_of_frame = atoi(s);
        } else if (strncmp(argv[i], "--numframes=", 12) == 0) {
            s = strstr(argv[i], "=") + 1;
            size_of_memory = atoi(s);
        } else if (strcmp(argv[i], "--progress") == 0) {
            show_progress = TRUE;
        }
    }

    if (infile_name == NULL) {
        infile = stdin;
    } else if (stat(infile_name, &infile_stat) == 0) {
        infile_size = (int)(infile_stat.st_size);
        /* If this fails, infile will be null */
        infile = fopen(infile_name, "r");
    }


    if (page_replacement_scheme == REPLACE_NONE ||
        size_of_frame <= 0 ||
        size_of_memory <= 0 ||
        infile == NULL)
    {
        fprintf(stderr, "usage: %s --framesize=<m> --numframes=<n>", argv[0]);
        fprintf(stderr, " --replace={fifo|lru|optimal} [--file=<filename>]\n");
        exit(1);
    }

    setup();

    while (fgets(buffer, MAX_LINE_LEN-1, infile)) {
        line_num++;
        if (strstr(buffer, ":")) {

            sscanf(buffer, "%c: %lx", &addr_type, &addr);

            LOG(buffer);

            if (addr_type == 'W') {
                is_write = TRUE;
            } else {
                is_write = FALSE;
            }

            if (resolve_address(addr, is_write) == -1) {
                error_resolve_address(addr, line_num);
            }
            mem_refs++;
            curr_time++;
            print(buffer);
        }

        if (show_progress) {
            display_progress(ftell(infile) * 100 / infile_size);
        }
    }


    teardown();
    output_report();

    fclose(infile);

    exit(0);
}
