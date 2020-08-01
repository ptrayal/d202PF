<?php

$WldDir = "world";
$newWldDirRoot = "world";
$newWldDir = "world/zon";

$fp1 = NULL;
$fp2 = NULL;
$fp3 = NULL;
$zoneNum = array("");

if (!is_dir($WldDir)) {
 mkdir($WldDir, 0755);
 if (!is_dir($WldDir)) {
  die ("Could not make directory $newWldDir.<br>");	
 }
}

$worldIndex = "/home/d20mud/generic/d20mud-v-0.90/lib/world/zon/index";

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
      
     $worldFile = "/home/d20mud/generic/d20mud-v-0.90/lib/world/zon/" . $worldFileOrig;	
     
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
         	  if ($linechar[0] == '@' || $linechar[0] == '#') {
         	   echo "Processing zone #$zoneNum... ";
		if ($linechar[0] == '@')
  		  $linechar = fgets($fp2);
         	   // zone vnum
             fwrite($fp3, $linechar);
             // Write zone Builders
             fwrite($fp3, "None.~\n");
             // Read zone name
             fwrite($fp3, fgets($fp2));
             // Write Zone Flags
             for ($i = 0; $i < count($zoneNum); $i++)
              fwrite($fp3, $zoneNum[$i]);
             if ($zoneNum[0] > 0)
              fwrite($fp3, "00");
             fwrite($fp3, " ");
             fwrite($fp3, fgets($fp2));
			 //Write new zone data
			 fwrite($fp3, "0 0 0 0 0\n0 0 0 0 0\n0 0 0 0 0\n");
            }
            else if ($linechar[0] == '*') {
             $trash = fgets($fp2);
            }
            else if (false && (($linechar[0] == 'M') || ($linechar[0] == 'O') || ($linechar[0] == 'G') || 
                    ($linechar[0] == 'P') || ($linechar[0] == 'E') || ($linechar[0] == 'D') || 
                    ($linechar[0] == 'R') || ($linechar[0] == 'T') || ($linechar[0] == 'V'))) {
              fwrite($fp3, $linechar[0]);
              for ($i = 1; $i < strlen($linechar); $i++) {
              	if (!ctype_digit($linechar[$i]) && $linechar[$i] != ' ' && $linechar[$i] != '-')
              	  break;
              	else
              	  fwrite($fp3, $linechar[$i]);
              }
              fwrite($fp3, "0 ");
              for ($i = $i; $i < strlen($linechar); $i++) {
              	if ($linechar[$i] == '\n')
              	  break;
              	else
              	  fwrite($fp3, $linechar[$i]);
              }
            	fwrite($fp3, "\n");
            }
           }
           fwrite($fp3, "S\n$\n");  
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
