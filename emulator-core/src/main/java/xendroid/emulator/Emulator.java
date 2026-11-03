// SPDX-License-Identifier: WTFPL
package xendroid.emulator;

import android.view.Surface;

import java.io.File;

public class Emulator {

    public static class ConfigFileException extends Exception{
    }

    public static class BootException extends Exception{
    }

    public static class Config{

        String config_path=null;
        private long n_handle;

        private native long native_open_config(String config_str) ;
        private native String native_close_config(long n_handle);
        private native long native_open_config_file(String config_path) ;
        private native String native_load_config_entry(long n_handle,String tag);

        private native String[] native_load_config_entry_ty_arr(long n_handle,String tag);
        private native void native_save_config_entry(long n_handle,String tag,String val);
        private native void native_remove_config_entry(long n_handle,String tag);
        private native boolean native_config_empty(long n_handle);

        private native void native_save_config_entry_ty_arr(long n_handle,String tag,String[] val);
        private native void native_close_config_file(long n_handle,String config_path);
        private native void native_free_config(long n_handle);

        /** The core's own changes of default values, one per row: date, category, name,
         *  type, old default, new default, tab-separated (emulator.cpp j_config_updates). */
        public static native String[] native_config_updates();
        public static Emulator.Config open_config_file(String config_path) throws Emulator.ConfigFileException
        {
            Emulator.Config config=new Emulator.Config();
            config.config_path=config_path;
            if((config.n_handle=config.native_open_config_file(config_path))==0)
                throw new Emulator.ConfigFileException();
            return config;
        }

        public static Emulator.Config open_config_from_string(String config_str) throws Emulator.ConfigFileException
        {
            Emulator.Config config=new Emulator.Config();
            if((config.n_handle=config.native_open_config(config_str))==0)
                throw new Emulator.ConfigFileException();
            return config;
        }

        public String load_config_entry(String tag)
        {
            return native_load_config_entry(n_handle,tag);
        }


        public String[] load_config_entry_ty_arr(String tag)
        {
            return native_load_config_entry_ty_arr(n_handle,tag);
        }
        public void save_config_entry(String tag,String val)
        {
            native_save_config_entry(n_handle,tag,val);
        }

        public void remove_config_entry(String tag) {
            native_remove_config_entry(n_handle, tag);
        }

        public boolean is_empty() {
            return native_config_empty(n_handle);
        }

        public void save_config_entry_ty_arr(String tag,String[] val)
        {
            native_save_config_entry_ty_arr(n_handle,tag,val);
        }
        public void close_config_file()
        {
            if(config_path==null)
                throw new RuntimeException("should use method close_config");
            native_close_config_file(n_handle,config_path);
        }

        public String close_config()
        {
            if(config_path!=null)
                throw new RuntimeException("should use method close_config_file");
            return native_close_config(n_handle);
        }

        /** Free the native table without serializing or writing anything. */
        public void free_config()
        {
            native_free_config(n_handle);
        }
    }

    public static class Path{
        String uri;
        int fd;
        public static Path from(String uri,int fd){
            Path p=new Path();
            p.uri=uri;
            p.fd=fd;
            return p;
        }
    }
    public boolean support_custom_driver(){
        try {
            return new File("/dev/kgsl-3d0").exists();
        }catch(Exception e){
            return false;
        }
    }

    public native void setup_game_path(String path);
    public native void setup_game_path(Emulator.Path path);
    public native void setup_surface(Surface sf);
    public native void boot() throws Emulator.BootException;

    public native void key_event(int key_code,boolean pressed,int value);

    public native void quit();

    public native boolean is_running();
    public native boolean is_paused();

    public native void pause();

    public native void resume();

    public native void change_surface(int w,int h);

    public native void flush_gpu_caches();

    /** Why the core gave up before the title ran, or null while it hasn't: "graphics" (Vulkan
     *  didn't start, as with a custom driver that has no device for this GPU), or "setup" or
     *  "subsystems" and the failing status in hex. */
    public native String boot_failure();
    
    public native void set_framerate_limit(int value);

    public native int get_framerate_limit();
}
