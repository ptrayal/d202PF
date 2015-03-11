<?php

$WldDir = "world";
$newWldDirRoot = "world";
$newWldDir = "world/obj";

$fp1 = NULL;
$fp2 = NULL;
$fp3 = NULL;
$zoneNum = array("");

if (!is_dir($WldDir)) {
 mkdir($WldDir, 0755);
 if (!is_dir($WldDir)) {
  die ("Could not make directory $WldDir.<br>");	
 }
}

$worldIndex = "/home/aol/building/lib/world/obj/index";

if (file_exists($worldIndex)) {
 if (is_file($worldIndex)) {
  if (is_readable($worldIndex)) {
   echo "Opening $worldIndex...";
   $fp1 = fopen($worldIndex, "r") or die ("Could not open $worldIndex for reading");
   echo " ...Success!\n";
   while (!feof($fp1)) {
   	
    $worldFileOrig = trim(fgets($fp1));
    
    for ($i = 0; $i < strlen($worldFileOrig); $i++) {
      if ($worldFileOrig[$i] == '.')
        break;
      $zoneNum[$i] = $worldFileOrig[$i];
    }

    if ($worldFileOrig != "$") {
      
      // Test Run Process Interruption
      //if ($worldFileOrig != "0.wld") die("Test run interruption\n");
      
     $worldFile = "/home/aol/building/lib/world/obj/" . $worldFileOrig;	
     
     if (file_exists($worldFile)) {
      if (is_file($worldFile)) {
       if (is_readable($worldFile)) {
        if (!is_dir($newWldDirRoot)) {
         mkdir($newWldDirRoot, 0755);
         if (!is_dir($newWldDir)) {
          mkdir($newWldDir, 0755);
          if (!is_dir($newWldDir)) {
           die ("Could not make directory $newWldDir.\n");	
          }
         }
        }
        touch($newWorldFile = $newWldDir . "/" . $worldFileOrig);
        if (file_exists($newWorldFile)) {
         if (is_file($newWorldFile)) {
          if (is_writable($newWorldFile)) {
           $fp2 = fopen($worldFile, "r") or die ("Could not open $worldIndex for reading");          	
           $fp3 = fopen($newWorldFile, "w") or die ("Could not open $newWorldFile for writing");       	
           echo "  Reading,converting and writing file data for $worldFile...\n";
           while (($linechar = (fgets($fp2))) && (!feof($fp2))) {
         	 if ($linechar[0] == '#') {
         	 echo "Processing obj $linechar... ";
         	 // mob vnum
             fwrite($fp3, $linechar);
             // write mob  aliases
             fwrite($fp3, fgets($fp2));
             // write short desc
             fwrite($fp3, fgets($fp2));
             // write long desc
             fwrite($fp3, fgets($fp2));
			 // write detailed description
			 $lineRead = fgets($fp2);
			 while (!strstr($lineRead, "~")) {
			   fwrite($fp3, $lineRead);
			   $lineRead = fgets($fp2);  
			 }
             fwrite($fp3, $lineRead);
                         // Write Object FLags
			 fwrite($fp3, fgets($fp2));
	
	         // Write Object V0-V15 information
                         $lineRead = fgets($fp2);
			 $tok = strtok($lineRead, " \n");
			 while ($tok) {
			   fwrite($fp3, $tok . " ");
			   $tok = strtok(" \n");
			 }
			 fwrite($fp3, "0 0 0 0 0 0 0 0 0 0 0 0\n");
			 
			 // Write  other object info
			 fwrite($fp3, fgets($fp2));
			 
            }
            else if ($linechar[0] == 'T' || $linechar[0] == '~' || $linechar[0] == '$') {
              fwrite($fp3, $linechar);
            }
			else if ($linechar[0] == 'E') {
			  fwrite($fp3, $linechar);
			  fwrite($fp3, fgets($fp2));
			  $lineRead = fgets($fp2);
			  while (!strstr($lineRead, "~")) {
			    fwrite($fp3, $lineRead);
				$lineRead = fgets($fp2);
			  }
			  fwrite($fp3, $lineRead);
			}
            else if ($linechar[0] == 'A') {
              fwrite($fp3, $linechar);
			  fwrite($fp3, fgets($fp2));
            }			
           }
           fwrite($fp3, "$\n");  
           echo " ...Done.\nSuccess!\n"; 
           if ($fp2) fclose($fp2);  
           if ($fp3) fclose($fp3);                       
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
     $worldFileOrig = trim(fgets($fp1));     
    }
   }
   if ($fp1) fclose($fp1);	 
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

?>
