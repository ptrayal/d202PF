<?php

$WldDir = "newworld";
$fp1 = NULL;
$fp2 = NULL;
$fp3 = NULL;
$recordCount = 0;
$triggerFound = false;	

if (!is_dir($WldDir)) {
 mkdir($WldDir, 0755);
 if (!is_dir($WldDir)) {
  die ("Could not make directory $newWldDir.<br>");	
 }
}

$worldIndex = "/home/aol/building/lib/world/mob/index";

if (file_exists($worldIndex)) {
 if (is_file($worldIndex)) {
  if (is_readable($worldIndex)) {
   echo "Opening $worldIndex...";
   $fp1 = fopen($worldIndex, "r") or die ("Could not open $worldIndex for reading");
   echo " ...Success!\n";
   while (!feof($fp1)) {
   	
    $worldFileOrig = trim(fgets($fp1, 1024));

    if ($worldFileOrig != "$") {
      
      // Test Run Process Interruption
      //if ($worldFileOrig != "0.wld") die("Test run interruption\n");
      
     $worldFile = "/home/aol/building/lib/world/mob/" . $worldFileOrig;	
     
     if (file_exists($worldFile)) {
      if (is_file($worldFile)) {
       if (is_readable($worldFile)) {
        $newWldDir = "newworld/mob"; 
        if (!is_dir($newWldDir)) {
         mkdir($newWldDir, 0755);
         if (!is_dir($newWldDir)) {
          die ("Could not make directory $newWldDir.\n");	
         }
        }
        touch($newWorldFile = $newWldDir . "/" . $worldFileOrig);
        if (file_exists($newWorldFile)) {
         if (is_file($newWorldFile)) {
          if (is_writable($newWorldFile)) {
           $fp2 = fopen($worldFile, "r") or die ("Could not open $worldIndex for reading");          	
           $fp3 = fopen($newWorldFile, "a") or die ("Could not open $newWorldFile for writing");       	
            echo "  Reading,converting and writing file data for $worldFile...\n";
           $linechar = fgetc($fp2);
           while (!feof($fp2)) {
		   
         	  if ($linechar == "#") {
		       if ($recordCount != 0 && !$triggerFound)
		        fwrite($fp3, "S\n");
			   $triggerFound = false;
			   $recordCount++;
         	   $rvnum = trim(fgets($fp2, 1024));
         	   echo "Processing mob #$rvnum... ";
         	   // room vnum
             fwrite($fp3, $linechar . $rvnum . "\n");
             // Read room name
             fwrite($fp3, fgets($fp2, 1024));
             // Read room name
             fwrite($fp3, fgets($fp2, 1024));
             // Read room name
             fwrite($fp3, fgets($fp2, 1024));
             fwrite($fp3, fgets($fp2, 1024));			 
             // room desc
             while (($mdescTemp =  fgetc($fp2)) != "~") {
              fwrite($fp3, $mdescTemp);
             }
             fwrite($fp3, "~");
             // mob and aff flags
             fwrite($fp3, fgets($fp2, 1024));
			 // mob hp and other stats
             fwrite($fp3, fgets($fp2, 1024));
			 // mob hp and other stats
             fwrite($fp3, fgets($fp2, 1024));				 
             // mob gold exp race and class
			 $mstats = fgets($fp2, 1024) . " 27";
             $mstats=ereg_replace("[\r\t\n\v]","",$mstats); 
			 $mstats = $mstats . "\n";
			 fwrite($fp3, $mstats);
             // default position and other ststs
             fwrite($fp3, fgets($fp2, 1024));

			 $mabils = 
			 "E\n" .
			 "Str: 14\n" .
			 "E\n" .
			 "Dex: 14\n" .
			 "E\n" .
			 "Int: 14\n" .
			 "E\n" .
			 "Wis: 14\n" .
			 "E\n" .
			 "Con: 14\n" .
			 "E\n" .
			 "Cha: 14\n";
			 fwrite($fp3, $mabils);			 
             // room zone, room flags, and room sector type
			 /*
             $trash = fgets($fp2, 1024);
             fwrite($fp3, fgets($fp2, 1024));
			 */
            }
            else if ($linechar == "T") {
          	 // dg script
			 if (!$triggerFound) {
			   fwrite($fp3, "S\n");
			   $triggerFound = true;
			 }
             fwrite($fp3, $linechar . fgets($fp2, 1024));
            }
            else if ($linechar == "$") {
			 if ($recordCount != 0 && !$triggerFound)
			   fwrite($fp3, "S\n");
             fwrite ($fp3, "$\n");
            }
			else if ($linechar == "S") {
			 $nextChar = fgetc($fp2);
			 if ($nextChar == "k") // Skindata
			  fwrite($fp3, "E\n" . $linechar . $nextChar . fgets($fp2, 1024));
			 else {
			  $linechar = $nextChar;
			  continue;
			 }
			}
			else {
              $linechar = fgetc($fp2);			
			  continue;
			}
            $linechar = fgetc($fp2);			
           }
           $recordCount = 0;			   
           echo " ...Done.\nSuccess!\n";         
          }
          else {
  	       echo $newWorldFile . "is not writable.<br>";
          }            
         }
         else {
 	        echo $newWorldFile . "is not a file<br>";
         }         
        }
        else {
	       echo $newWorldFile . "does not exist<br>";
        }        
       }
       else {
  	    echo $worldFile . "is not readable.<br>";
       }       
      }
      else {
 	     echo $worldFile . "is not a file<br>";
      }
     }
     else {
	    echo $worldFile . "does not exist<br>";
     }
     if ($fp2) fclose($fp2);  
     if ($fp3) fclose($fp3);       
     $worldFileOrig = trim(fgets($fp1, 1024));     
    }
   }
  }
  else {
  	echo $worldIndex . "is not readable.<br>";
  }
 }
 else {
 	echo $worldIndex . "is not a file<br>";
 }
}
else {
	echo $worldIndex . "does not exist<br>";
}
if ($fp1) fclose($fp1);	 
?>
