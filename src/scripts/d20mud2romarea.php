<?php

	$outputFile = null;				// The final rom area file
	$sourceFile = null;				// The current file being read
	$indexFile = null;				// The d20mud index file that lists the source file names
	
	$worldDir = "/home/d20mud/live/lib/world/";		// The root location of the source files
	$fullOutputDir = "/home/d20mud/live/lib/romAreas/"; // The full path of the output directory
	$outputDir = "romAreas";						// The root location of the rom area output files
	$fileNumber = array("");						// The name of the source file obtained from the index
	$oString = array("");							// output string for file processing
	$resetBuf = "";								// a buffer to contain reset data so we don't have to read the .zon file twice
	
	// Create the output directory if it is not already there
	
	if (!is_dir($outputDir)) {
		mkdir($outputDir, 0755);
		if (!is_dir($outputDir)) {
			die ("Could not make directory " .  $outputDir . ".\n");	
		}
	}
	
	// #AREA
	
	$index = "/home/d20mud/live/lib/world/zon/index";
	
	if (file_exists($index)) {
		if (is_file($index)) {
			if (is_readable($index)) {
				echo "Opening " . $index . "...\n";
				$indexFile = fopen($index, "r") or die ("Could not open " . $index . " for reading\n");
				echo " ...Success!\n";
				while (!feof($indexFile)) {
   	
					$sourceFileName = trim(fgets($indexFile));
    
					for ($i = 0; $i < strlen($sourceFileName); $i++) {
						if ($sourceFileName[$i] == '.')
						break;
						$fileNumber[$i] = $sourceFileName[$i];
					}

					echo $fileNumber . "\n";
					
					if ($fileNumber != "$") {
	
						$sourceFileName = $worldDir . "zon/" . $sourceFileName;	
     
						if (file_exists($sourceFileName)) {
							if (is_file($sourceFileName)) {
								if (is_readable($sourceFileName)) {
									$ouputFileName = ($fullOutputDir . $fileNumber . ".are");
									touch($outputFileName);
									if (file_exists($outputFileName)) {
										if (is_file($outputFileName)) {
											if (is_writable($outputFileName)) {
												$sourceFile = fopen($sourceFileName, "r") or die ("Could not open " . $sourceFileName . " for reading");          	
												$outputFile = fopen($outputFileName, "w") or die ("Could not open " . $outputFileName . " for writing");       	
												fwrite($outputFile, "#AREA\n");
												echo "  Reading, converting and writing #AREA file data from " . $sourceFileName . "...\n";
												
												// filename
												$linechar = fgets($sourceFile);
												for ($i = 1; $i < strlen($linechar); $i++)
													$oString[$i - 1] = $linechar[$i];
												fwrite($outputFile, $oString . ".are~\n");
												
												// read and discard builders data
												$linechar = fgets($sourceFile);
												
												// Write Area Name
												fwrite($outputFile, $oString . "~\n");
												
												// area description
												$linechar = fgets($sourceFile);
												fwrite($outputFile, "{1 90} " . $linechar . "\n");
												
												// zone vnums
												fscanf($sourceFile, "%d %d %s", $num1, $num2, $oString);
												fwrite($outputFile, "%d %d", $num1, $num2);
												
												for ($i = 0; $i < 4; $i++)
													$linechar = fgets($sourceFile);
												
												while (($linechar = (fgets($sourceFile))) && (!feof($sourceFile))) {
													if ($linechar[0] == "S" || $linechar[0] == "P")
														continue;
													else if ($linechar[0] == "$")
														break;
													else if ($linechar[0] == "M") {
														$retval = sscanf($linechar, "%c %d %d %d %d %d %s", $char1, $num1, $num2, $num3, $num4, $num5, $oString);
														$resetBuf .= "M 0 " . $num2 . " " . $num4 . " " . $num3 . " " .  $num3 . "\n";
													}
													else if ($linechar[0] == "O") {
													}
												}
												fclose($sourceFile);
												fclose($outputFile);
											}
										}
									}
								}
							}
						}
					}
				}
				fclose($indexFile);
			}
		}
	}
	
	



?>