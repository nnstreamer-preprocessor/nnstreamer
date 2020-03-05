#include <gst/gst.h>
#include <gst/app/gstappsrc.h>

#include <stdint.h>
#include <stdio.h>
#include <math.h>

int want = 1;
double frame[630][64]={0,};
double frame2[1024][640]={0,};//


static void prepare_buffer(GstAppSrc* appsrc, char *index) {

  //static gboolean white = FALSE;
  static GstClockTime timestamp = 0;
  GstBuffer *buffer;
  guint size;
  GstFlowReturn ret;


  ////////////////////////////////////

  char filename[14];
  char s2[13]="data_point/";
  sprintf(filename,"00000000%s.txt",index);
  char *filename2=strcat(s2, filename);
  printf("%s\n", filename2);

  FILE* fp;
  fp = fopen(filename2,"r");
  double read_num, x, y, z;
  double xy_theta, z_theta, range, xy_theta_round, y_theta_round;
  double dif_chk_xy, dif_chk_y;
  double val = 57.2958;

  int cnt=1;

  while(fgetc(fp) != EOF){

    fscanf(fp, "%lf", &read_num);

    if(cnt%4==0){
        //calculate
        xy_theta=atan(y/x)*val+45;
        z_theta=atan(z/x)*val+5;
        if( 0<z_theta & z_theta<=64 & 0< xy_theta & xy_theta<=90 ){
               
            range=round(sqrt(x*x+y*y+z*z)*100)/100;
            xy_theta=xy_theta*7;
            xy_theta_round=round(xy_theta);
            y_theta_round=round(z_theta);
               
            frame[(int)xy_theta][(int)z_theta]=range;       
            //printf("range=%f  z_theta=%f  xy_theta=%f \n", range, z_theta , xy_theta);//frame=%f\n , frame[(int)xy_theta][(int)z_theta]             
        }

      /**
        x=(round(x)+320);
        y=(round(y)+612);
        if(x<640 & y<1024){
          frame2[(int)x][(int)y]=z;
          printf("zz=%f",z);
        }
      */
               
    }else if(cnt%4==1){
        //x
        x=read_num;
        //printf("x=%f",x);
    }else if(cnt%4==2){
        //y
        y=read_num;
        //printf(" y=%f",y);
    }else if(cnt%4==3){
        //z
        z=read_num;
        //printf(" z=%f\n",z);
    }
    cnt=cnt+1;
  }

  printf("number of point cnt=%d\n",cnt/4);

  fclose(fp);
  

  /**
  int col = 630;
  int row = 64;
  int cnt2=0;
  for(int i=0; i<col; i++){
      for(int j=0; j<row; j++){ 
            if(frame[i][j]==0){
                 //printf("%5.2f ",frame[i][j]);
                 cnt2=cnt2+1;
            }
      }
      //printf("\n");
  }
  printf("number of point cnt2=%d\n",cnt2);
  */
  ////////////////////////////////////


  if (!want) return;
  want = 0;
 

  /////////////////////
  size = 630*64*2;
  buffer = gst_buffer_new_wrapped_full( 0, (gpointer)frame, size, 0, size, NULL, NULL );
  ////////////////////////


  GST_BUFFER_PTS (buffer) = timestamp;
  GST_BUFFER_DURATION (buffer) = gst_util_uint64_scale_int (1, GST_SECOND, 4);

  timestamp += GST_BUFFER_DURATION (buffer);

  ret = gst_app_src_push_buffer(appsrc, buffer);

  if (ret != GST_FLOW_OK) {
    /* something wrong, stop pushing */
    // g_main_loop_quit (loop);
  }
}

static void cb_need_data (GstElement *appsrc, guint unused_size, gpointer user_data) {
  //prepare_buffer((GstAppSrc*)appsrc);
  want = 1;
}

gint main (gint argc, gchar *argv[]) {

  GstElement *pipeline, *appsrc, *conv, *videosink;


  /////

  char *file_list[11]={"00","01","02","03","04","05","06","07","08","09","10"};
  /**
  for(int i=10; i<100;i++){
      char txt[2];
      sprintf(txt, "%d", i);
      file_list[i]=txt;
  }*/
  /////


  /* init GStreamer */
  gst_init (&argc, &argv);

  /* setup pipeline */
  pipeline = gst_pipeline_new ("pipeline");
  appsrc = gst_element_factory_make ("appsrc", "source");
  conv = gst_element_factory_make ("videoconvert", "conv");
  videosink = gst_element_factory_make ("xvimagesink", "videosink");

  /* setup */
  g_object_set (G_OBJECT (appsrc), "caps",
  		gst_caps_new_simple ("video/x-raw",
				     "format", G_TYPE_STRING, "RGB16",
				     "width", G_TYPE_INT, 630,
				     "height", G_TYPE_INT, 64,
				     "framerate", GST_TYPE_FRACTION, 25, 1,
				     NULL), NULL);
  gst_bin_add_many (GST_BIN (pipeline), appsrc, conv, videosink, NULL);
  gst_element_link_many (appsrc, conv, videosink, NULL);

  /* setup appsrc */
  g_object_set (G_OBJECT (appsrc),
		"stream-type", 0, // GST_APP_STREAM_TYPE_STREAM
		"format", GST_FORMAT_TIME,
        "is-live", TRUE,
    NULL);
  g_signal_connect (appsrc, "need-data", G_CALLBACK (cb_need_data), NULL);

  /* play */
  gst_element_set_state (pipeline, GST_STATE_PLAYING);
  

  /**
  while (1) {
    prepare_buffer((GstAppSrc*)appsrc);
    g_main_context_iteration(g_main_context_default(),FALSE);
  }*/



  ////////////////////sizeof(file_list)/8

  for(int i=0; i<sizeof(file_list)/8; i++) {
    prepare_buffer((GstAppSrc*)appsrc, file_list[i]);
    g_main_context_iteration(g_main_context_default(),FALSE);
  }
  /////////////////////////




  /* clean up */
  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (GST_OBJECT (pipeline));

  return 0;
}