/*========== my_main.c ==========

  This is the only file you need to modify in order
  to get a working mdl project (for now).

  my_main.c will serve as the interpreter for mdl.
  When an mdl script goes through a lexer and parser, 
  the resulting operations will be in the array op[].

  Your job is to go through each entry in op and perform
  the required action from the list below:

  push: push a new origin matrix onto the origin stack
  pop: remove the top matrix on the origin stack

  move/scale/rotate: create a transformation matrix 
                     based on the provided values, then 
		     multiply the current top of the
		     origins stack by it.

  box/sphere/torus: create a solid object based on the
                    provided values. Store that in a 
		    temporary matrix, multiply it by the
		    current top of the origins stack, then
		    call draw_polygons.

  line: create a line based on the provided values. Store 
        that in a temporary matrix, multiply it by the
	current top of the origins stack, then call draw_lines.

  save: call save_extension with the provided filename

  display: view the image live
  
  jdyrlandweaver
  =========================*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "parser.h"
#include "symtab.h"
#include "y.tab.h"

#include "matrix.h"
#include "ml6.h"
#include "display.h"
#include "draw.h"
#include "stack.h"

/*======== void first_pass() ==========
  Inputs:   
  Returns: 

  Checks the op array for any animation commands
  (frames, basename, vary)
  
  Should set num_frames and basename if the frames 
  or basename commands are present

  If vary is found, but frames is not, the entire
  program should exit.

  If frames is found, but basename is not, set name
  to some default value, and print out a message
  with the name being used.

  jdyrlandweaver
  ====================*/
void first_pass() {
  //in order to use name and num_frames
  //they must be extern variables
  extern int num_frames;
  extern char name[128];
  int baseSet = 0; 
  int varySet = 0;
  num_frames = 0;
  
  printf("FIRST PASS\n");
  int i = 0;
  for (i=0;i<lastop;i++) {
    //printf("%d: \n", i);
    switch (op[i].opcode){
      case FRAMES:
        num_frames = op[i].op.frames.num_frames;
        break;
      case BASENAME:
        strcpy(name, op[i].op.basename.p->name);
        //printf("Name: %s\n", name);
        baseSet = 1;
        break;
      case VARY:
        varySet = 1;
        break;
    }
  }
  if (num_frames <= 0 && varySet){
    printf("Frame number (%d) out of range.\n",num_frames);//tested
    exit(0);
  }
  if (!baseSet){
    printf("Basename not specified, setting basename to \"simple\".\n");//tested
    strcpy(name, "simple");
  }
  return;
}

/*======== struct vary_node ** second_pass() ==========
  Inputs:   
  Returns: An array of vary_node linked lists

  In order to set the knobs for animation, we need to keep
  a seaprate value for each knob for each frame. We can do
  this by using an array of linked lists. Each array index
  will correspond to a frame (eg. knobs[0] would be the first
  frame, knobs[2] would be the 3rd frame and so on).

  Each index should contain a linked list of vary_nodes, each
  node contains a knob name, a value, and a pointer to the
  next node.

  Go through the opcode array, and when you find vary, go 
  from knobs[0] to knobs[frames-1] and add (or modify) the
  vary_node corresponding to the given knob with the
  appropirate value. 

  jdyrlandweaver
  ====================*/
struct vary_node ** second_pass() {
  //struct vary_node {
  //  
  //  char name[128];
  //  double value;
  //  struct vary_node *next;
  //};
  printf("SECOND PASS\n");
  struct vary_node ** knobs = malloc(sizeof(struct vary_node *) * num_frames);
  struct vary_node * knob;
  int i = 0;
  int j = 0;
  int start_frame;
  int end_frame;
  double start_val;
  double end_val;
  double current = 0;
  double increment = 0;
  int knobNum = 0;

  
  for (i=0;i<lastop;i++) {
    switch (op[i].opcode){
      case VARY:
        start_frame = op[i].op.vary.start_frame;
        end_frame = op[i].op.vary.end_frame;
        start_val = op[i].op.vary.start_val;
        end_val = op[i].op.vary.end_val;
        current = start_val;
        increment = (end_val - start_val) / (end_frame - start_frame);
        
        //error checking
        if (start_frame > end_frame || start_frame < 0 || end_frame > num_frames){
          printf("Invalid vary range, exiting\n");
          exit(0);
        }
        
        //loop and enter frame values
        printf("vary: %s\n", op[i].op.vary.p->name);
        for (j = start_frame; j < end_frame; j++){
          knob = (struct vary_node *) malloc(sizeof(struct vary_node));  
          strcpy(knob->name, op[i].op.vary.p->name);       
          knob->value = current;
          knob->next = knobs[j];
          knobs[j] = knob;
          current += increment;
          //printf("%s\n", knob->name);
          //printf("%f\n", knob->value);   
          //printf("%f\n", current);
        }
        break;
    }
  }
  return knobs;
}


/*======== void print_knobs() ==========
Inputs:   
Returns: 

Goes through symtab and display all the knobs and their
currnt values

jdyrlandweaver
====================*/
void print_knobs() {
  
  int i;

  printf( "ID\tNAME\t\tTYPE\t\tVALUE\n" );
  for ( i=0; i < lastsym; i++ ) {

    if ( symtab[i].type == SYM_VALUE ) {
      printf( "%d\t%s\t\t", i, symtab[i].name );

      printf( "SYM_VALUE\t");
      printf( "%6.2f\n", symtab[i].s.value);
    }
  }
}


/*======== void my_main() ==========
  Inputs: 
  Returns: 

  This is the main engine of the interpreter, it should
  handle most of the commadns in mdl.

  If frames is not present in the source (and therefore 
  num_frames is 1, then process_knobs should be called.

  If frames is present, the enitre op array must be
  applied frames time. At the end of each frame iteration
  save the current screen to a file named the
  provided basename plus a numeric string such that the
  files will be listed in order, then clear the screen and
  reset any other data structures that need it.

  Important note: you cannot just name your files in 
  regular sequence, like pic0, pic1, pic2, pic3... if that
  is done, then pic1, pic10, pic11... will come before pic2
  and so on. In order to keep things clear, add leading 0s
  to the numeric portion of the name. If you use sprintf, 
  you can use "%0xd" for this purpose. It will add at most
  x 0s in front of a number, if needed, so if used correctly,
  and x = 4, you would get numbers like 0001, 0002, 0011,
  0487

  jdyrlandweaver
  ====================*/
void my_main() {
  int anim = 0;
  struct vary_node * knob;
  struct vary_node ** knobs;
  
  first_pass();
  if (num_frames > 0){
    knobs = second_pass();
    anim = 1;
  }else{
    num_frames = 1;
  }

  int i;
  struct matrix *tmp;
  struct stack *systems;
  screen t;
  color g;
  double step = 0.1;
  double theta;
  
  systems = new_stack();
  tmp = new_matrix(4, 1000);
  clear_screen( t );
  g.red = 0;
  g.green = 0;
  g.blue = 0;

  int f;
  double cknob = 1;
  char frame[128];
  
  for (f = 0; f < num_frames; f++){
    if(anim){
      printf("ANIM\n");
      knob = knobs[f];
      while ( knob != NULL ) {
	      set_value(lookup_symbol(knob->name), knob->value);
        printf("knob_name: %s\n", knob->name);
        printf("knob_value: %f\n", knob->value);
	      knob = knob->next;
      }
    }
    for (i=0;i<lastop;i++) {
      printf("%d: ",i);
      switch (op[i].opcode){
      	case SPHERE:
      	  printf("Sphere: %6.2f %6.2f %6.2f r=%6.2f",
      		 op[i].op.sphere.d[0],op[i].op.sphere.d[1],
      		 op[i].op.sphere.d[2],
      		 op[i].op.sphere.r);
      	  if (op[i].op.sphere.constants != NULL)
      	    {
      	      printf("\tconstants: %s",op[i].op.sphere.constants->name);
      	    }
      	  if (op[i].op.sphere.cs != NULL)
      	    {
      	      printf("\tcs: %s",op[i].op.sphere.cs->name);
      	    }
      	  add_sphere(tmp, op[i].op.sphere.d[0],
      		     op[i].op.sphere.d[1],
      		     op[i].op.sphere.d[2],
      		     op[i].op.sphere.r, step);
      	  matrix_mult( peek(systems), tmp );
      	  draw_polygons(tmp, t, g);
      	  tmp->lastcol = 0;
      	  break;
      	case TORUS:
      	  printf("Torus: %6.2f %6.2f %6.2f r0=%6.2f r1=%6.2f",
      		 op[i].op.torus.d[0],op[i].op.torus.d[1],
      		 op[i].op.torus.d[2],
      		 op[i].op.torus.r0,op[i].op.torus.r1);
      	  if (op[i].op.torus.constants != NULL)
      	    {
      	      printf("\tconstants: %s",op[i].op.torus.constants->name);
      	    }
      	  if (op[i].op.torus.cs != NULL)
      	    {
      	      printf("\tcs: %s",op[i].op.torus.cs->name);
      	    }
      	  add_torus(tmp,
      		    op[i].op.torus.d[0],
      		    op[i].op.torus.d[1],
      		    op[i].op.torus.d[2],
      		    op[i].op.torus.r0,op[i].op.torus.r1, step);
      	  matrix_mult( peek(systems), tmp );
      	  draw_polygons(tmp, t, g);
      	  tmp->lastcol = 0;	  
      	  break;
      	case BOX:
      	  printf("Box: d0: %6.2f %6.2f %6.2f d1: %6.2f %6.2f %6.2f",
      		 op[i].op.box.d0[0],op[i].op.box.d0[1],
      		 op[i].op.box.d0[2],
      		 op[i].op.box.d1[0],op[i].op.box.d1[1],
      		 op[i].op.box.d1[2]);
      	  if (op[i].op.box.constants != NULL)
      	    {
      	      printf("\tconstants: %s",op[i].op.box.constants->name);
      	    }
      	  if (op[i].op.box.cs != NULL)
      	    {
      	      printf("\tcs: %s",op[i].op.box.cs->name);
      	    }
      	  add_box(tmp,
      		  op[i].op.box.d0[0],op[i].op.box.d0[1],
      		  op[i].op.box.d0[2],
      		  op[i].op.box.d1[0],op[i].op.box.d1[1],
      		  op[i].op.box.d1[2]);
      	  matrix_mult( peek(systems), tmp );
      	  draw_polygons(tmp, t, g);
      	  tmp->lastcol = 0;
      	  break;
      	case LINE:
      	  printf("Line: from: %6.2f %6.2f %6.2f to: %6.2f %6.2f %6.2f",
      		 op[i].op.line.p0[0],op[i].op.line.p0[1],
      		 op[i].op.line.p0[1],
      		 op[i].op.line.p1[0],op[i].op.line.p1[1],
      		 op[i].op.line.p1[1]);
      	  if (op[i].op.line.constants != NULL)
      	    {
      	      printf("\n\tConstants: %s",op[i].op.line.constants->name);
      	    }
      	  if (op[i].op.line.cs0 != NULL)
      	    {
      	      printf("\n\tCS0: %s",op[i].op.line.cs0->name);
      	    }
      	  if (op[i].op.line.cs1 != NULL)
      	    {
      	      printf("\n\tCS1: %s",op[i].op.line.cs1->name);
      	    }
      	  break;
      	case MOVE:
      	  printf("Move: %6.2f %6.2f %6.2f",
      		 op[i].op.move.d[0],op[i].op.move.d[1],
      		 op[i].op.move.d[2]);
           cknob = 1;
           if (op[i].op.move.p != NULL){
             printf("\tknob: %s",op[i].op.move.p->name);
             cknob = lookup_symbol(op[i].op.move.p->name)->s.value;
             printf(" || move: %f\n", lookup_symbol(op[i].op.move.p->name)->s.value);
           }
      	  tmp = make_translate( op[i].op.move.d[0] * cknob,
      				op[i].op.move.d[1] * cknob,
      			  op[i].op.move.d[2] * cknob);
          //tmp = make_translate(250, 250, 250);
      	  matrix_mult(peek(systems), tmp);
      	  copy_matrix(tmp, peek(systems));
      	  tmp->lastcol = 0;
      	  break;
      	case SCALE:
      	  printf("Scale: %6.2f %6.2f %6.2f",
      		 op[i].op.scale.d[0],op[i].op.scale.d[1],
      		 op[i].op.scale.d[2]);
          cknob = 1;
          if (op[i].op.scale.p != NULL){
            printf("\tknob: %s",op[i].op.scale.p->name);
            cknob = lookup_symbol(op[i].op.scale.p->name)->s.value;
            printf(" || scale: %f", lookup_symbol(op[i].op.scale.p->name)->s.value);
          }
      	  tmp = make_scale(op[i].op.scale.d[0] * cknob,
      			    op[i].op.scale.d[1] * cknob,
      			    op[i].op.scale.d[2] * cknob);
      	  matrix_mult(peek(systems), tmp);
      	  copy_matrix(tmp, peek(systems));
      	  tmp->lastcol = 0;
      	  break;
      	case ROTATE:
      	  printf("Rotate: axis: %6.2f degrees: %6.2f",
      		 op[i].op.rotate.axis,
      		 op[i].op.rotate.degrees);
      	  theta =  op[i].op.rotate.degrees * (M_PI / 180);
          if (op[i].op.rotate.p != NULL){
            printf("\tknob: %s",op[i].op.rotate.p->name);
            theta  *= lookup_symbol(op[i].op.rotate.p->name)->s.value;
            printf(" || rotate: %f", lookup_symbol(op[i].op.rotate.p->name)->s.value);
          }
      	  if (op[i].op.rotate.axis == 0 )
      	    tmp = make_rotX( theta );
      	  else if (op[i].op.rotate.axis == 1 )
      	    tmp = make_rotY( theta );
      	  else
      	    tmp = make_rotZ( theta );
      	  matrix_mult(peek(systems), tmp);
      	  copy_matrix(tmp, peek(systems));
      	  tmp->lastcol = 0;
      	  break;
      	case PUSH:
      	  printf("Push");
      	  push(systems);
      	  break;
      	case POP:
      	  printf("Pop");
      	  pop(systems);
      	  break;
      	case SAVE:
      	  printf("Save: %s",op[i].op.save.p->name);
      	  save_extension(t, op[i].op.save.p->name);
      	  break;
      	case DISPLAY:
      	  printf("Display");
      	  display(t);
      	  break;
      }
      printf("\n");
    }
    if(anim){
      sprintf(frame, "anim/%s%03d.png", name, f);
      save_extension(t,frame);
      printf("Saving frame #%d under name: %s.\n", f, frame);
      clear_screen(t);
      tmp->lastcol = 0;
      systems = new_stack();
    }
  }
  if (anim){
    printf("ANIMATING\n");
    make_animation(name);
  }
}
