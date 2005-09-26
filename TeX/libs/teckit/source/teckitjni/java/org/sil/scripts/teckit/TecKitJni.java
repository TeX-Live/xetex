/*
 ------------------------------------------------------------------------
Copyright (C) 2002-2004 Keith Stribley

Distributable under the terms of either the Common Public License or the
GNU Lesser General Public License, as specified in the LICENSING.txt file.

File: Engine.cp
Responsibility: Keith Stribley
Last reviewed: Not yet.

Description:
    Implements a JNI interface to the TECkit conversion engine.
-------------------------------------------------------------------------*/

package org.sil.scripts.teckit;
import java.net.URL;
import java.io.File;
/**
 *
 * @author  keith
 */
public class TecKitJni
{
    private static final String LIBRARY = "TecKitJni";
    private static boolean libraryLoaded = false;
    public static boolean loadLibrary(File libraryPath)
    {
        if (!libraryLoaded) 
        {
            try
            {
                File library = new File
                    (libraryPath, 
                    System.mapLibraryName(LIBRARY));
                System.load(library.getAbsolutePath());
                System.out.println("Loaded " + System.mapLibraryName(LIBRARY));
                libraryLoaded = true;
            }
            catch (UnsatisfiedLinkError e)
            {
                System.out.println(e);
                System.out.println(e.getMessage() + " " + 
                    System.mapLibraryName(LIBRARY) + " " + 
                    System.getProperty("java.library.path"));
            }
            catch (Exception e)
            {
                e.printStackTrace();
            }
        }
        return libraryLoaded;
    }
    /**
     * Checks whether the library is already loaded.
     * @return true if the library has been loaded.
     */
    public static boolean isLibraryLoaded() { return libraryLoaded; }
    
    /** Creates a new instance of TecKitJni */
    public TecKitJni()
    {
        
    }
    /**
     * Creates a converter for a given tec file.
     * @param path the full path to the .tec file
     * @param toUnicode true if the direction is from bytes to Unicode
     * false if the direction is from Unicode to bytes
     * @return id of the converter created. This must be used in subsequent 
     * invocations to convert.
     */
    public native long createConverter(String path, boolean toUnicode);
    /**
     * Converts the input byte array using the specified converter, which must
     * have already be created with #createConverter.
     * @param convId returned by #createConverter
     * @param inputArray of bytes or UTF-8 encoded unicode.
     * @return byte array of converted string as UTF-8 or bytes depending on direction.
     */
    public native byte [] convert(long convId, byte [] inputArray);
    /**
     * Flushes the converter.
     * @param convId returned by #createConverter
     */
    public native void flush(long convId);
    /**
     * Destroys the specified converter, releasing any resources.
     * @param convId returned by #createConverter
     */
    public native void destroyConverter(long convId);
}
