
/*
 * Copyright (c) 2006-2013 The StarBED Project  All rights reserved.
 *
 * See the file 'LICENSE' for licensing information.
 *
 */

/************************************************************************
 *
 * QOMET Emulator Implementation
 *
 * File name: xml_jpgis.c
 * Function:  Load objects from a JPGIS topology description
 *
 * Authors: Khin Thida Latt, Razvan Beuran
 *
 * $Id: xml_jpgis.c 146 2013-06-20 00:50:48Z razvan $
 *
 ***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "xml_jpgis.h"
#include "message.h"
#include "global.h"
#include "generic.h"


/////////////////////////////////////////////////
// xml_jpgis functions
/////////////////////////////////////////////////

// init the xml_jpgis structure
void
xml_jpgis_init (struct xml_jpgis_class *xml_jpgis,
		struct scenario_class *scenario,
		struct object_class *objects, int object_number)
{
  int i;

  xml_jpgis->object_found = FALSE;
  xml_jpgis->coordinate_found = FALSE;

  strcpy (xml_jpgis->coordinate_text, "");

  xml_jpgis->scenario = scenario;
  xml_jpgis->objects = objects;
  xml_jpgis->object_number = object_number;
  xml_jpgis->object_j = 0;
  xml_jpgis->coordinate_i = 0;
  xml_jpgis->error = FALSE;

  xml_jpgis->latitude = -HUGE_VAL;
  xml_jpgis->longitude = -HUGE_VAL;

  for (i = 0; i < object_number; i++)
    if (objects[i].load_all_from_region == TRUE)
      {
	xml_jpgis->load_all_from_region = TRUE;
	break;
      }
    else
      xml_jpgis->load_all_from_region = FALSE;
}


/////////////////////////////////////////////////
// Various XML parsing functions
/////////////////////////////////////////////////

// callback used when the start tag of an element is reached
static void XMLCALL
xml_jpgis_start_element (void *user_data, const char *el, const char **attr)
{
  // get a pointer to the user data in our XML object
  struct xml_jpgis_class *xml_jpgis = (struct xml_jpgis_class *) user_data;

  int i, j;

  // search for all recognized elements (objects)
  if ((strcmp (el, BUILDING_EDGE_LABEL) == 0) ||
      (strcmp (el, ROAD_EDGE_LABEL) == 0))
    {
      // reset coordinate found tag
      xml_jpgis->coordinate_found = FALSE;

      for (i = 0; attr[i]; i += 2)
	{
	  if (strcmp (attr[i], "id") == 0)
	    {
	      DEBUG ("  Found %s with %s='%s'\n", el, attr[i], attr[i + 1]);

	      for (j = 0; j < xml_jpgis->object_number; j++)
		// check whether object needs to be loaded from JPGIS file
		if (xml_jpgis->objects[j].load_from_jpgis_file == TRUE)
		  // check object name
		  if (strcmp (xml_jpgis->objects[j].name, attr[i + 1]) == 0)
		    {
		      // check whether the object has the appropriate type
		      if (((strcmp (el, BUILDING_EDGE_LABEL) == 0) &&
			   xml_jpgis->objects[j].type == BUILDING_OBJECT) ||
			  ((strcmp (el, ROAD_EDGE_LABEL) == 0) &&
			   xml_jpgis->objects[j].type == ROAD_OBJECT))
			{
			  INFO ("\tObject to be loaded='%s'",
				xml_jpgis->objects[j].name);
			  xml_jpgis->object_found = TRUE;
			  xml_jpgis->object_j = j;
			  xml_jpgis->coordinate_i = 0;
			  xml_jpgis->objects[j].vertex_number = 0;
			}
		      else
			WARNING ("Found object '%s' has is not of expected \
type", xml_jpgis->objects[j].name);
		    }

	      // check whether we are in region adding mode, and the current
	      // object has not been added in the loop above
	      if (xml_jpgis->load_all_from_region == TRUE
		  && xml_jpgis->object_found == FALSE)
		{
		  char temp_name[MAX_STRING], *temp_name1;

		  strncpy (temp_name, attr[i + 1], MAX_STRING - 1);
		  temp_name1 = temp_name;
		  object_init (&(xml_jpgis->temp_object), temp_name1,
			       DEFAULT_ENVIRONMENT_NAME);
		  xml_jpgis->temp_object.make_polygon = FALSE;

		  // only two types of object can be found here;
		  // test which is the case
		  if (strcmp (el, BUILDING_EDGE_LABEL) == 0)
		    xml_jpgis->temp_object.type = BUILDING_OBJECT;
		  else if (strcmp (el, ROAD_EDGE_LABEL) == 0)
		    xml_jpgis->temp_object.type = ROAD_OBJECT;

		  DEBUG ("Object '%s' to be loaded if in region.",
			 xml_jpgis->temp_object.name);
		  //object_print(&(xml_jpgis->temp_object));

		  xml_jpgis->object_found = TRUE;
		  xml_jpgis->object_j = -1;
		  xml_jpgis->coordinate_i = 0;
		  xml_jpgis->temp_object.vertex_number = 0;
		}
	    }
	}
    }
  // check whether a coordinate element was found
  else if (strcmp (el, "jps:coordinate") == 0)
    {
      // check whether we are in an object of interest, 
      // or we are in region loading mode
      if (xml_jpgis->object_found == TRUE
	  || xml_jpgis->load_all_from_region == TRUE)
	{
	  // reset flags related to coordinate parsing
	  xml_jpgis->latitude = -HUGE_VAL;
	  xml_jpgis->longitude = -HUGE_VAL;
	  xml_jpgis->coordinate_found = TRUE;
	  strcpy (xml_jpgis->coordinate_text, "");
	}
      else			// discard the coordinate
	xml_jpgis->coordinate_found = FALSE;
    }
  else				// discard the coordinate
    xml_jpgis->coordinate_found = FALSE;

  // increase XML parsing tree depth
  xml_depth++;
}

// callback used when the end tag of an element is reached
static void XMLCALL
xml_jpgis_end_element (void *user_data, const char *el)
{
  // get a pointer to the user data in our XML object
  struct xml_jpgis_class *xml_jpgis = (struct xml_jpgis_class *) user_data;

  // helping pointer
  struct object_class *crt_object =
    &(xml_jpgis->objects[xml_jpgis->object_j]);

  int j;

  // skip processing when in error state
  if (xml_jpgis->error == TRUE)
    return;

  // decrease XML parsing tree depth
  xml_depth--;

  // search for all recognized elements
  if ((strcmp (el, BUILDING_EDGE_LABEL) == 0) ||
      (strcmp (el, ROAD_EDGE_LABEL) == 0))
    {
      // check whether the object that just ended was imported
      if (xml_jpgis->object_found == TRUE)
	{
	  DEBUG ("Added %d coordinates to object.\n",
		 crt_object->vertex_number);

	  // loading objects from region
	  if (xml_jpgis->object_j == -1)
	    {
	      DEBUG
		("Check whether the object is in any of the defined regions, and add it to scenario object array if this is the case.");
#ifdef MESSAGE_DEBUG
	      object_print (&(xml_jpgis->temp_object));
#endif

	      for (j = 0; j < xml_jpgis->object_number; j++)
		{
		  // check whether object needs to be loaded from region
		  if (xml_jpgis->load_all_from_region == FALSE)
		    continue;

		  if ((strcmp (el, BUILDING_EDGE_LABEL) == 0)
		      && (xml_jpgis->objects[j].type == ROAD_OBJECT))
		    {
		      DEBUG
			("Ignoring 'building' object for 'road' region '%s'.",
			 xml_jpgis->objects[j].name);
		      continue;
		    }

		  if ((strcmp (el, ROAD_EDGE_LABEL) == 0)
		      && (xml_jpgis->objects[j].type == BUILDING_OBJECT))
		    {
		      DEBUG
			("Ignoring 'road' object for 'building' region '%s'.",
			 xml_jpgis->objects[j].name);
		      continue;
		    }

		  DEBUG ("Current element is '%s', region is type '%d'",
			 el, xml_jpgis->objects[j].type);

		  // object is of right type, check if it is in region
		  double rx1, ry1, rx2, ry2;
		  int vertex_match = 0;
		  int k;

		  rx1 = xml_jpgis->objects[j].vertices[0].c[0];
		  ry1 = xml_jpgis->objects[j].vertices[0].c[1];
		  rx2 = xml_jpgis->objects[j].vertices[2].c[0];
		  ry2 = xml_jpgis->objects[j].vertices[2].c[1];

		  DEBUG
		    ("Check if object '%s' is in region '%s' with type=%d [(%f,%f); (%f,%f)]",
		     xml_jpgis->temp_object.name, xml_jpgis->objects[j].name,
		     xml_jpgis->objects[j].type, rx1, ry1, rx2, ry2);
		  for (k = 0; k < xml_jpgis->temp_object.vertex_number; k++)
		    {
		      if (point_in_object
			  (xml_jpgis->temp_object.vertices[k].c[0],
			   xml_jpgis->temp_object.vertices[k].c[1], rx1, ry1,
			   rx2, ry2))
			{
			  DEBUG ("Vertex #%d IS in region", k);
			  vertex_match++;
			}
		      else
			DEBUG ("Vertex #%d IS NOT in region", k);
		    }

		  if (vertex_match == xml_jpgis->temp_object.vertex_number)
		    {
		      DEBUG
			("All vertices of object '%s' are in region '%s' => adding to scenario",
			 xml_jpgis->temp_object.name,
			 xml_jpgis->objects[j].name);
		      strncpy (xml_jpgis->temp_object.environment,
			       xml_jpgis->objects[j].environment,
			       MAX_STRING - 1);
		      // copy height information
		      xml_jpgis->temp_object.height =
			xml_jpgis->objects[j].height;


		      if (scenario_add_object
			  (xml_jpgis->scenario,
			   &xml_jpgis->temp_object) == NULL)
			{
			  WARNING ("Cannot add object '%s' to scenario",
				   xml_jpgis->temp_object.name);
			  xml_jpgis->error = TRUE;
			  return;
			}
		    }
		}
	    }
	  else			// loading simple objects
	    {
	      // reset the loading flag of the object
	      crt_object->load_from_jpgis_file = FALSE;
	    }

	  /* moved the related functionality to xml_scenario.c so that
	     the user-defined 'make_polygon' flag can take precedence

	     // since JPGIS objects use a specific convention to indicate  
	     // that they are polygons (first and last vertex coordinates 
	     // are equal), we need to set our make_polygon flag to false,
	     // so as to prevent artificial/undesired polygon creation
	     crt_object->make_polygon = FALSE;
	   */
	}

      // reset object found flag
      xml_jpgis->object_found = FALSE;
    }
  // check whether the object that just ended was a coordinate we imported
  else if ((strcmp (el, "jps:coordinate") == 0) &&
	   xml_jpgis->object_found == TRUE)
    {
      char *space_ptr;

      struct object_class *target_object;

      // identify space position
      space_ptr = strstr (xml_jpgis->coordinate_text, " ");
      //DEBUG("space_ptr='%s'", space_ptr);

      // convert second part of the string
      if (space_ptr != NULL)
	{
	  xml_jpgis->longitude = double_value (space_ptr);
	  space_ptr[0] = '\0';

	  // convert first part of the string
	  //DEBUG("coordinate_text='%s'", xml_jpgis->coordinate_text);
	  xml_jpgis->latitude = double_value (xml_jpgis->coordinate_text);
	}

      // check whether both coordinates were correctly imported
      if (xml_jpgis->longitude == -HUGE_VAL
	  || xml_jpgis->latitude == -HUGE_VAL)
	{
	  // restore space character for printing
	  space_ptr[0] = ' ';
	  WARNING ("Error converting the string '%s' to object coordinates",
		   xml_jpgis->coordinate_text);
	  xml_jpgis->error = TRUE;
	}

      if (xml_jpgis->object_j == -1)
	{
	  target_object = &(xml_jpgis->temp_object);
	}
      else
	target_object = &(xml_jpgis->objects[xml_jpgis->object_j]);

      // try to add newly parsed coordinate to the current object
      if (xml_jpgis->coordinate_i < MAX_VERTICES)
	{
	  struct coordinate_class point_blh, point_xyz;

	  // since JPGIS coordinate system is not cartesian, we convert
	  // latitude & longitude to x & y before storing
	  DEBUG ("Converting object coordinates: latitude=%.9f \
longitude=%.9f", xml_jpgis->latitude, xml_jpgis->longitude);

	  // copy coordinates to structure
	  point_blh.c[0] = xml_jpgis->latitude;
	  point_blh.c[1] = xml_jpgis->longitude;
	  point_blh.c[2] = 0;

	  // transform lat & long to x & y
	  ll2en (&point_blh, &point_xyz);

	  // copy converted data
	  (target_object->vertices[xml_jpgis->coordinate_i]).c[0]
	    = point_xyz.c[0];
	  (target_object->vertices[xml_jpgis->coordinate_i]).c[1]
	    = point_xyz.c[1];

	  // FIXME: set z coordinate to a good value (use altitude
	  // value from JPGIS file for point_blh.c[2] above?!)
	  (target_object->vertices[xml_jpgis->coordinate_i]).c[2]
	    = point_xyz.c[2];

	  // update coordinate index and object vertex number
	  xml_jpgis->coordinate_i++;
	  target_object->vertex_number++;
	}
      else
	{
	  WARNING ("Maximum number of coordinates (%d) reached when \
loading object", MAX_VERTICES);
	  xml_jpgis->error = TRUE;
	}
    }
}

// callback used to parse the text inside an element (i.e.,
// included between the start and end tags of that element)
void XMLCALL
xml_jpgis_element_text (void *user_data, const char *xmlData, int length)
{
  // get a pointer to the user data in our XML object
  struct xml_jpgis_class *xml_jpgis = (struct xml_jpgis_class *) user_data;

  // check whether the text is inside a coordinate we want to import,
  // and the text has a length larger than 1 (equivalent to non-empty string)
  if (xml_jpgis->coordinate_found == TRUE && length > 1)
    {
      // if there is still enough space, add the text to our
      // parsing buffer
      if (strlen (xml_jpgis->coordinate_text) + length + 1 < MAX_STRING)
	strncat (xml_jpgis->coordinate_text, xmlData, length);
      else
	{
	  WARNING ("Coordinate text exceeded maximum size (%d)", MAX_STRING);
	  xml_jpgis->error = TRUE;
	}
    }
}

/*
// callback used to parse the text inside an element (i.e.,
// included between the start and end tags of that element)
void XMLCALL xml_jpgis_element_text(void *user_data, 
				    const char *xmlData, int length)
{
  // get a pointer to the user data in our XML object
  struct xml_jpgis_class *xml_jpgis=(struct xml_jpgis_class *)user_data;

  char* tempString=NULL;
  char* space_ptr;

  // check whether the text is inside a coordinate we want to import,
  // and the text has a length larger than 1 (equivalent to non-empty string)
  if(xml_jpgis->coordinate_found==TRUE && length>1)
    {
      tempString = (char *)malloc(length+1);  // memory allocation
      strncpy(tempString, xmlData, length);
      tempString[length] = '\0';
  
      // convert string value of latitude and longitude to double
      //longitude = strtod(strstr(tempString, " ")+1, tail_ptr);
      //latitude =  strtod(tempString, tail_ptr);

      DEBUG("Converting the string '%s' to object coordinates...",
	      tempString);

      if(xml_jpgis->latitude_imported == 0)
	{
	  space_ptr = strstr(tempString, " ");
	  //DEBUG("space_ptr='%s'", space_ptr);

	  if(space_ptr != NULL)
	    {
	      xml_jpgis->longitude = double_value(space_ptr);
	      space_ptr[0] = '\0';
	      xml_jpgis->longitude_imported = 1;
	    }
	  //DEBUG("tempString='%s'", tempString);
	  xml_jpgis->latitude = double_value(tempString);
	  xml_jpgis->latitude_imported = 1;
	}
      else
	{
	  xml_jpgis->longitude = double_value(tempString);
	  xml_jpgis->longitude_imported = 1;
	}

      if(xml_jpgis->longitude==-HUGE_VAL || xml_jpgis->latitude==-HUGE_VAL)
	{
       	  WARNING("Error converting the string '%s' to object coordinates",
		  tempString);
	  xml_jpgis->error = TRUE;
	}

      free(tempString); // deallocate the memory

      // try to add newly parsed coordinate to the current object
      if(xml_jpgis->coordinate_i<MAX_VERTICES)
	{
	  struct coordinate_class point_blh, point_xyz;
	 
	  if(xml_jpgis->latitude_imported==TRUE && 
	     xml_jpgis->longitude_imported==TRUE)
	    {
	      // since JPGIS coordinate system is not cartesian, we convert
	      // latitude & longitude to x & y before storing
	      DEBUG("Converting object coordinates: latitude=%.9f \
longitude=%.9f", xml_jpgis->latitude, xml_jpgis->longitude);

	      // copy coordinates to structure
	      point_blh.c[0] = xml_jpgis->latitude;
	      point_blh.c[1] = xml_jpgis->longitude;
	      
	      // transform lat & long to x & y
	      ll2en(&point_blh, &point_xyz);
	      
	      // copy converted data
	      ((xml_jpgis->objects[xml_jpgis->object_j]).
	       vertices[xml_jpgis->coordinate_i]).c[0] = point_xyz.c[0];
	      ((xml_jpgis->objects[xml_jpgis->object_j]).
	       vertices[xml_jpgis->coordinate_i]).c[1] = point_xyz.c[1];
	      
	      // update coordinate index and object vertex number
	      xml_jpgis->coordinate_i++;
	      (xml_jpgis->objects[xml_jpgis->object_j]).vertex_number++;
	    }
	}
      else
	{
	  WARNING("Maximum number of coordinates (%d) reached when loading object", MAX_VERTICES);
	  xml_jpgis->error = TRUE;
	} 
    }
}
*/

/////////////////////////////////////////////////
// Main XML parsing function 
/////////////////////////////////////////////////

// top-level function of this module used to load the objects;
// return SUCCESS on success, ERROR on error
int
xml_jpgis_load_objects (struct scenario_class *scenario,
			struct object_class *objects, int object_number,
			char *jpgis_filename)
{
  FILE *jpgis_file;
  int error_status = SUCCESS;


  // object used to store state while parsing
  struct xml_jpgis_class xml_jpgis;

  XML_Parser xml_parser = XML_ParserCreate (NULL);

  xml_jpgis_init (&xml_jpgis, scenario, objects, object_number);

  INFO ("Loading objects from file '%s'...", jpgis_filename);

  if (xml_parser == NULL)
    {
      fprintf (stderr, "Couldn't allocate memory for parser\n");
      return ERROR;
    }

  jpgis_file = fopen (jpgis_filename, "r");
  if (jpgis_file == NULL)
    {
      fprintf (stderr, "ERROR: Could not open file '%s'.\n", jpgis_filename);
      goto ERROR_HANDLE;
    }

  XML_SetElementHandler (xml_parser, xml_jpgis_start_element,
			 xml_jpgis_end_element);
  XML_SetCharacterDataHandler (xml_parser, xml_jpgis_element_text);

  // set the user data pointer to the scenario object
  XML_SetUserData (xml_parser, &xml_jpgis);

  for (;;)
    {
      int done;
      int len;

      len = (int) fread (xml_buffer, 1, BUFFER_SIZE, jpgis_file);
      if (ferror (jpgis_file))
	{
	  fprintf (stderr, "Read error\n");
	  exit (-1);
	}
      done = feof (jpgis_file);

      if (XML_Parse (xml_parser, xml_buffer, len, done) == XML_STATUS_ERROR)
	{
	  fprintf (stderr, "Parse error at line %" XML_FMT_INT_MOD "u:\n%s\n",
		   (long unsigned int) XML_GetCurrentLineNumber (xml_parser),
		   XML_ErrorString (XML_GetErrorCode (xml_parser)));
	  goto ERROR_HANDLE;
	}

      if (done)
	break;
    }

  // if no errors occurred, skip next instruction
  goto FINAL_HANDLE;

ERROR_HANDLE:
  error_status = ERROR;


  ////////////////////////////////////////////////////////////
  // finalizing phase

FINAL_HANDLE:

  if (jpgis_file != NULL)
    fclose (jpgis_file);

  XML_ParserFree (xml_parser);

  if (error_status == ERROR || xml_jpgis.error == TRUE)
    {
      WARNING ("Error encountered when loading objects from JPGIS file '%s' \
(see the WARNING messages above)", jpgis_filename);
      error_status = ERROR;
    }
  else
    {
#ifdef MESSAGE_DEBUG
      int i;

      DEBUG ("====================\n");
      DEBUG ("List of objects in scenario:");
      for (i = 0; i < object_number; i++)
	object_print (&objects[i]);
      DEBUG ("====================\n");
#endif
    }

  return error_status;
}
