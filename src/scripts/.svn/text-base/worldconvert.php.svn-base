<?php

$WldDir = "newworld";

if (!is_dir($WldDir)) {
 mkdir($WldDir, 0755);
 if (!is_dir($WldDir)) {
  die ("Could not make directory $newWldDir.<br>");	
 }
}

$worldIndex = "/home/aol/production/lib/world/wld/index";

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
      
     $worldFile = "/home/aol/production/lib/world/wld/" . $worldFileOrig;	
     
     if (file_exists($worldFile)) {
      if (is_file($worldFile)) {
       if (is_readable($worldFile)) {
        $newWldDir = "world/wld"; 
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
         	   $rvnum = trim(fgets($fp2, 1024));
         	   echo "Processing room #$rvnum... ";
         	   // room vnum
             fwrite($fp3, $linechar . $rvnum . "\n");
             // Read room name
             fwrite($fp3, fgets($fp2, 1024));
             // room desc
             while (($rdescTemp =  fgetc($fp2)) != "~") {
              fwrite($fp3, $rdescTemp);
             }
             fwrite($fp3, "~\n");
             // room zone, room flags, and room sector type
             $trash = fgets($fp2, 1024);
             fwrite($fp3, fgets($fp2, 1024));
            }
            else if ($linechar == "D") {
             fwrite($fp3, "D" . fgets($fp2, 1024));
             while (($exdescTemp = fgetc($fp2)) != "~") {
              fwrite($fp3, $exdescTemp);
             }
             fwrite($fp3, "~\n");
             $trash = fgets($fp2, 1024);
             fwrite($fp3,  fgets($fp2, 1024));
             $exitflags = trim(fgets($fp2, 1024));
             fwrite($fp3, $exitflags . " 0 0 0 0 0 0 0 0\n");
             
            }
            else if ($linechar == "E") {
             fwrite($fp3, "E\n");          
             fwrite($fp3, fgets($fp2, 1024));
             while (($edescTemp =  fgetc($fp2)) != "~") {
              fwrite($fp3, $edescTemp);
             }
             fwrite($fp3, "~\n");
            }
            else if ($linechar == "T") {
          	 // dg script
             fwrite($fp3, $linechar . fgets($fp2, 1024));
            }
            else if ($linechar == "S") {
             fwrite($fp3, "S\n");
            }
            else if ($linechar == "$") {
             fwrite ($fp3, "$\n");
            }
            $linechar = fgetc($fp2);
           }
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
