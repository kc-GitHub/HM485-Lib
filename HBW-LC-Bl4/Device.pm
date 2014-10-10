package HM485::Device;

use strict;
use warnings;
use Data::Dumper;
use Scalar::Util qw(looks_like_number);
use POSIX qw(ceil);

use Cwd qw(abs_path);
use FindBin;
use lib abs_path("$FindBin::Bin");
use lib::HM485::Constants;

use vars qw {%attr %defs %modules}; #supress errors in Eclipse EPIC

# prototypes
sub parseForEepromData($;$$);

my %deviceDefinitions;
my %models = ();

=head2
	Initialize all devices
	Load available device files
=cut
sub init () {
	my $retVal      = '';
	my $devicesPath = $main::attr{global}{modpath} . HM485::DEVICE_PATH;

	if (opendir(DH, $devicesPath)) {
		HM485::Util::logger(HM485::LOGTAG_HM485, 3, 'HM485: Loading available device files');
		HM485::Util::logger(HM485::LOGTAG_HM485, 3, '=====================================');
		foreach my $m (sort readdir(DH)) {
			next if($m !~ m/(.*)\.pm$/);
			
			my $deviceFile = $devicesPath . $m;
			if(-r $deviceFile) {
				HM485::Util::logger(HM485::LOGTAG_HM485, 3, 'Loading device file: ' .  $deviceFile);
				my $includeResult = do $deviceFile;
	
				if($includeResult) {
					foreach my $dev (keys %HM485::Devicefile::definition) {
						$deviceDefinitions{$dev} = $HM485::Devicefile::definition{$dev};
					}
				} else {
					HM485::Util::logger(
						HM485::LOGTAG_HM485, 3,
						'HM485: Error in device file: ' . $deviceFile . ' deactivated:' . "\n $@"
					);
				}
				%HM485::Devicefile::definition = ();

			} else {
				HM485::Util::logger(
					HM485::LOGTAG_HM485, 1,
					'HM485: Error loading device file: ' .  $deviceFile
				);
			}
		}
		closedir(DH);
	
		if (scalar(keys %deviceDefinitions) < 1 ) {
			return 'HM485: Warning, no device definitions loaded!';
		}
	
		initModels();
	} else {
		$retVal = 'HM485: ERROR! Can\'t read devicePath: ' . $devicesPath . $!;
	}
		
	return $retVal;
}

=head2
	Initialize all loaded models
=cut
sub initModels () {
	foreach my $deviceKey (keys %deviceDefinitions) {

		if ($deviceDefinitions{$deviceKey}{models}) {
			foreach my $modelKey (keys (%{$deviceDefinitions{$deviceKey}{models}})) {
				if ($deviceDefinitions{$deviceKey}{models}{$modelKey}{type}) {
					$models{$modelKey}{model} = $modelKey;
					$models{$modelKey}{name} = $deviceDefinitions{$deviceKey}{models}{$modelKey}{name};
					$models{$modelKey}{type} = $deviceDefinitions{$deviceKey}{models}{$modelKey}{type};
					
					my $minFW = $deviceDefinitions{$deviceKey}{models}{$modelKey}{minFW_version};
					$minFW = $minFW ? $minFW : 0;
					$models{$modelKey}{versionDeviceKey}{$minFW} = $deviceKey; 
				}
			}
		}
	}
}

=head2
	Get device key depends on firmware version
=cut
sub getDeviceKeyFromHash($) {
	my ($hash) = @_;

	my $retVal = '';
	if ($hash->{MODEL}) {
		my $model    = $hash->{MODEL};
		my $fw  = $hash->{FW_VERSION} ? $hash->{FW_VERSION} : 0;
		my $fw1 = $fw ? int($fw) : 0;
		my $fw2 = ($fw * 100) - int($fw) * 100;

		my $fwVersion = hex(sprintf (
			'%02X%02X',
			($fw1 ? $fw1 : 0),
			($fw2 ? $fw2 : 0)
		));
		
		foreach my $version (keys (%{$models{$model}{versionDeviceKey}})) {
			if ($version <= $fwVersion) {
				$retVal = $models{$model}{versionDeviceKey}{$version};
			} else {
				last;
			}
		}
	}
	
	return $retVal;
}





### we should rework below this ###

=head2
	Get the model from numeric hardware type
	
	@param	int      the numeric hardware type
	@return	string   the model
=cut
sub getModelFromType($) {
	my ($hwType) = @_;

	my $retVal = undef;
	foreach my $model (keys (%models)) {
		if (exists($models{$model}{type}) && $models{$model}{type} == $hwType) {
			$retVal = $model;
			last;
		}
	}

	return $retVal;
}

=head2 getModelName
	Title		: getModelName
	Usage		: my $modelName = getModelName();
	Function	: Get the model name from $models hash
	Returns 	: string
	Args 		: nothing
=cut
sub getModelName($) {
	my ($hwType) = @_;
	
	my $retVal = $hwType;
	if (defined($models{$hwType}{'name'})) {
		$retVal = $models{$hwType}{'name'};
	}
	
	return $retVal;
}

=head2 getModelList
	Title		: getModelList
	Usage		: my $modelList = getModelList();
	Function	: Get a list of models from $models hash
	Returns 	: string
	Args 		: nothing
=cut
sub getModelList() {
	my @modelList;
	
	foreach my $type (keys %models) {
		if ($models{$type}{'model'}) {
			push (@modelList, $models{$type}{'model'});
		}
	}

	return join(',', @modelList);
}

sub getChannelBehaviour($) {
	my ($hash) = @_;

	my $retVal = undef;
	
	my ($hmwId, $chNr) = HM485::Util::getHmwIdAndChNrFromHash($hash);

	if ($chNr) {
		my $deviceKey     = getDeviceKeyFromHash($hash);
		my $chType        = getChannelType($deviceKey, $chNr);
		if ($deviceKey && $chType) {

			my $channelConfig  = getValueFromDefinitions(
				$deviceKey . '/channels/' . $chType .'/'
			);

			if ($channelConfig->{special_param}{behaviour}{physical}{address_id}) {
				my $chConfig = HM485::ConfigurationManager::getConfigFromDevice(
					$hash, $chNr
				);

				my @posibleValuesArray = split(',', $chConfig->{behaviour}{posibleValues});
			
				# Trim all items in the array
				@posibleValuesArray = grep(s/^\s*(.*)\s*$/$1/, @posibleValuesArray);

				my $value = $chConfig->{behaviour}{value};
				
				$retVal = $posibleValuesArray[$value];
			}
		}
	}
	return $retVal;
}


=head2 getHwTypeList
	Title		: getHwTypeList
	Usage		: my $modelHwList = getHwTypeList();
	Function	: Get a list of model harwaretypes from $models hash
	Returns 	: string
	Args 		: nothing
=cut
sub getHwTypeList() {
	return join(',', sort keys %models);
}

sub getValueFromDefinitions ($) {
	my ($path) = @_;
	my $retVal = undef;
	my @pathParts = split('/', $path);
	
	my %definitionPart = %deviceDefinitions;
	my $found = 1;
	foreach my $part (@pathParts) {
		if (ref($definitionPart{$part}) eq 'HASH') {
			%definitionPart = %{$definitionPart{$part}};
		} else {
			if ($definitionPart{$part}) {
				$retVal = $definitionPart{$part};
			} else {
				$retVal = undef;
				$found = 0;			
			}
			last;
		}
	}
	
	if (!defined($retVal) && $found) {
		$retVal = {%definitionPart};
	}

	return $retVal
}

sub getChannelType($$) {
	my ($deviceKey, $chNo) = @_;
	$chNo = int($chNo);
	
	my $retVal = undef;

	my $channels = getValueFromDefinitions($deviceKey . '/channels/');
	my @chArray  = getChannelsByModelgroup($deviceKey);
	foreach my $channel (@chArray) {
		my $chStart = int($channels->{$channel}{id});
		my $chCount = int($channels->{$channel}{count});
		if (($chNo == 0 && $chStart == 0) ||
		    ($chNo >= $chStart && $chNo < ($chStart + $chCount) && $chStart > 0)) {

			$retVal = $channel;
			last;
		}
	}
	
	return $retVal;
}

=head2
	Parse incomming frame data and split to several values
	
	@param	hash	the hash of the IO device
	@param	string	message to parse
=cut
sub parseFrameData($$$) {
	my ($hash, $data, $actionType) = @_;

	my $deviceKey = HM485::Device::getDeviceKeyFromHash($hash);
	my $frameData = getFrameInfos($deviceKey, $data, 1, 'from_device');


#	print Dumper($frameData);
	my $retVal    = convertFrameDataToValue($hash, $deviceKey, $frameData);

	return $retVal;
}

=head2
	Get all infos of current frame data
	
	@param	string	the deviceKey
	@param	string	the frame data to parse
	@param	boolean	optinal value identify the frame as event 
	@param	string	optional frame direction (from or to device)
=cut
sub getFrameInfos($$;$$) {
	my ($deviceKey, $data, $event, $dir) = @_;
	
	my $frameType = hex(substr($data, 0,2));
	my %retVal;

	my $frames = getValueFromDefinitions($deviceKey . '/frames/');
	if ($frames) {
		foreach my $frame (keys %{$frames}) {
			my $fType  = $frames->{$frame}{type};
			my $fEvent = $frames->{$frame}{event} ? $frames->{$frame}{event} : 0;
			my $fDir   = $frames->{$frame}{dir} ? $frames->{$frame}{dir} : 0;
			
			if ($frameType == $fType &&
			   (!defined($event) || $event == $fEvent) &&
			   (!defined($event) || $dir eq $fDir) ) {

				my $chField = ($frames->{$frame}{ch_field} - 9) * 2;
				my $params = translateFrameDataToValue($data, $frames->{$frame}{params});
				if (defined($params)) {
					%retVal = (
						ch     => sprintf ('%02d' , hex(substr($data, $chField, 2)) + 1),
						params => $params,
						type   => $fType,
						event  => $frames->{$frame}{event} ? $frames->{$frame}{event} : 0,
						id     => $frame
					);
					last;
				}
			}
		}
	}
	
	return \%retVal;
}

sub getValueFromEepromData($$$;$) {
	my ($hash, $configHash, $adressStart, $wholeByte) = @_;
	$wholeByte = $wholeByte ? 1 : 0;

	my $adressStep = $configHash->{address_step} ? $configHash->{address_step}  : 1;
	my ($adrId, $size, $littleEndian) = getPhysical($hash, $configHash, $adressStart, $adressStep);

	my $retVal = '';
	if (defined($adrId)) {
		my $data = HM485::Device::getRawEEpromData(
			$hash, int($adrId), ceil($size), 0, $littleEndian
		);
		my $eepromValue = 0;

		my $adrStart = (($adrId * 10) - (int($adrId) * 10)) / 10;
		$adrStart    = ($adrStart < 1 && !$wholeByte) ? $adrStart: 0;
		$size        = ($size < 1 && $wholeByte) ? 1 : $size;
		
		$eepromValue = getValueFromHexData($data, $adrStart, $size);
		$retVal = dataConversion($eepromValue, $configHash->{conversion}, 'from_device');
		my $default = $configHash->{logical}{'default'};
		if (defined($default)) {
			if ($size == 1) {
				$retVal = ($eepromValue != 0xFF) ? $retVal : $default;

			} elsif ($size == 2) {
				$retVal = ($eepromValue != 0xFFFF) ? $retVal : $default;

			} elsif ($size == 4) {
				$retVal = ($eepromValue != 0xFFFFFFFF) ? $retVal : $default;
			}
		}
	}

	return $retVal;
}

sub getPhysical($$$$) {
	my ($hash, $configHash, $adressStart, $adressStep) = @_;
	
	my $adrId      = 0;
	my $size       = 0;

	my ($hmwId, $chNr) = HM485::Util::getHmwIdAndChNrFromHash($hash);

	my $deviceKey      = HM485::Device::getDeviceKeyFromHash($hash);
	my $chType        = HM485::Device::getChannelType($deviceKey, $chNr);
	my $chConfig       = HM485::Device::getValueFromDefinitions(
		$deviceKey . '/channels/' . $chType .'/'
	);
	my $chId = $chNr - $chConfig->{id};

	# we must check if spechial params exists.
	# Then adress_id and step retreve from spechial params 
	my $valId = $configHash->{physical}{value_id};
	if ($valId) {
		my $spConfig       = $chConfig->{special_param}{$valId};

		$adressStep  = $spConfig->{physical}{address_step} ? $spConfig->{physical}{address_step}  : 0;
		$size        = $spConfig->{physical}{size}         ? $spConfig->{physical}{size} : 1;
		$adrId       = $spConfig->{physical}{address_id}   ? $spConfig->{physical}{address_id} : 0;
#		$adrId       = $adrId + ($chId * $adressStep * ceil($size));
		$adrId       = $adrId + ($chId * $adressStep);

	} else {
		$size       = $configHash->{physical}{size} ? $configHash->{physical}{size} : 1;
		$adrId      = $configHash->{physical}{address_id} ? $configHash->{physical}{address_id} : 0;
#		$adrId      = $adrId + $adressStart + ($chId * $adressStep * ceil($size));
		$adrId      = $adrId + $adressStart + ($chId * $adressStep);
	}
	
	my $littleEndian = ($configHash->{physical}{endian} && $configHash->{physical}{endian} eq 'little') ? 1 : 0;

	return ($adrId, $size, $littleEndian);
}

sub translateFrameDataToValue($$) {
	my ($data, $params) = @_;
	$data = pack('H*', $data);

	my $dataValid = 1;
	my %retVal;
	if ($params) {
		foreach my $param (keys %{$params}) {
			$param = lc($param);

			my $id    = ($params->{$param}{id} - 9);
			my $size  = ($params->{$param}{size});
			my $value = getValueFromHexData($data, $id, $size);
#print Dumper(unpack ('H*',$data));
#print Dumper($value);

			my $constValue = $params->{$param}{const_value};
			if (!defined($constValue) || $constValue eq $value) {
				$retVal{$param}{val} = $value;
			} else {
				$dataValid = 0;
				last
			}
		}
	}
	
	return $dataValid ? \%retVal : undef;
}

sub getValueFromHexData($;$$) {
	my ($data, $start, $size) = @_;
#print Dumper(unpack ('H*',$data), $start, $size);

	$start = $start ? $start : 0;
	$size  = $size ? $size : 1;

	my $retVal;

	if (isInt($start) && $size >=1) {
		$retVal = hex(unpack ('H*', substr($data, $start, $size)));
	} else {
		my $bitsId = ($start - int($start)) * 10;
		my $bitsSize  = ($size - int($size)) * 10;
		$retVal = ord(substr($data, int($start), 1));
		$retVal = subBit($retVal, $bitsId, $bitsSize);
	}

	return $retVal;
}

sub convertFrameDataToValue($$$) {
	my ($hash, $deviceKey, $frameData) = @_;

	if ($frameData->{ch}) {
		foreach my $valId (keys %{$frameData->{params}}) {
			my $valueMap = getChannelValueMap($hash, $deviceKey, $frameData, $valId);
			if ($valueMap) {
				$frameData->{params}{$valId}{val} = dataConversion(
					$frameData->{params}{$valId}{val},
					$valueMap->{conversion},
					'from_device'
				);

				$frameData->{value}{$valueMap->{name}} = valueToControl(
					$valueMap,
					$frameData->{params}{$valId}{val},
					
				);
			}
		}
	}

	return $frameData;
}

=head2
	Map values to control specific values

	@param	hash    hash of parameter config
	@param	number    the data value
	
	@return string    converted value
=cut
sub valueToControl($$) {
	my ($paramHash, $value) = @_;
	my $retVal = $value;

	my $control = $paramHash->{control};
	my $valName = $paramHash->{name};

	if ($control) {
		if ($control eq 'switch.state') {
			my $threshold = $paramHash->{conversion}{threshold};
			$threshold = $threshold ? int($threshold) : 1;
			$retVal = ($value > $threshold) ? 'on' : 'off';

		} elsif ($control eq 'dimmer.level') {
			$retVal = $value;

		} elsif (index($control, 'button.') > -1) {
			$retVal = $valName . ' ' . $value;

		} else {
			$retVal = $value;
		}

	} else {
		$retVal = $value;
	}
	
	return $retVal;
}

sub onOffToState($$) {
	my ($stateHash, $cmd) = @_;

	my $state = 0;
	my $conversionHash = $stateHash->{conversion};

	if ($cmd eq 'on' && defined($conversionHash->{true})) {
		$state = $conversionHash->{true};
	} elsif ($cmd eq 'off' && defined($conversionHash->{false})) {
		$state = $conversionHash->{false};
	}

	return $state;
}

sub valueToState($$$$) {
	my ($chType, $valueHash, $valueKey, $value) = @_;
	
	my $factor = $valueHash->{conversion}{factor} ? int($valueHash->{conversion}{factor}) : 1;
	
	my $state = int($value * $factor);
#print Dumper($chType, $valueHash, $valueKey, $value);
#return 0;

	return $state;
}

sub buildFrame($$$) {
	my ($hash, $frameType, $frameData) = @_;
	my $retVal = '';

	if (ref($frameData) eq 'HASH') {
		my ($hmwId, $chNr) = HM485::Util::getHmwIdAndChNrFromHash($hash);
		my $devHash        = $main::modules{HM485}{defptr}{substr($hmwId,0,8)};
		my $deviceKey      = HM485::Device::getDeviceKeyFromHash($devHash);

		my $frameHash = HM485::Device::getValueFromDefinitions(
			$deviceKey . '/frames/' . $frameType .'/'
		);

		$retVal = sprintf('%02X%02X', $frameHash->{type}, $chNr-1);

#print Dumper($frameHash);
#print Dumper($frameData);		
		foreach my $key (keys %{$frameData}) {
			my $valueId = $frameData->{$key}{physical}{value_id};

			if ($valueId && ref($frameHash->{params}{$valueId}) eq 'HASH') {
				my $paramLen = $frameHash->{params}{$valueId}{size} ? int($frameHash->{params}{$valueId}{size}) : 1;
				$retVal.= sprintf('%0' . $paramLen * 2 . 'X', $frameData->{$key}{value});
			}
		}
	}

	return $retVal;
}

=head2
	Convert values specifyed in config files

	@param	number    the value to convert
	@param	hast      convertConfig hash
	@param	string    the converting direction
	
	@return string    converted value
=cut
sub dataConversion($$;$) {
	my ($value, $convertConfig, $dir) = @_;
	
#	print Dumper($convertConfig);
	my $retVal = $value;
	if (ref($convertConfig) eq 'HASH') {
		$dir = ($dir && $dir eq 'to_device') ? 'to_device' : 'from_device';

		my $type = $convertConfig->{type};

		if (ref($convertConfig->{value_map}) eq 'HASH' && $convertConfig->{value_map}{type}) {
			foreach my $key (keys %{$convertConfig->{value_map}}) {
				my $valueMap = $convertConfig->{value_map}{$key};
				if (ref($valueMap) eq 'HASH') {

					if ($convertConfig->{value_map}{type} eq 'integer_integer_map') {
						my $valParam  = $valueMap->{parameter_value} ? $valueMap->{parameter_value} : 0;
						my $valDevice = $valueMap->{device_value} ? $valueMap->{device_value} : 0;
	
						if ($dir eq 'to_device' && $valueMap->{to_device}) {
							$retVal = ($value == $valParam) ? $valDevice : $retVal;
						} elsif ($dir eq 'from_device' && $valueMap->{from_device}) {
							$retVal = ($value == $valDevice) ? $valParam : $retVal;
						}
					}
				}
			}
		}

		if ($type eq 'float_integer_scale' || $type eq 'integer_integer_scale') {
			my $factor = $convertConfig->{factor} ? $convertConfig->{factor} : 1;
			my $offset = $convertConfig->{offset} ? $convertConfig->{offset} : 0;
			$factor = ($type eq 'float_integer_scale') ? $factor : 1;
#my $t = $retVal;
			if ($dir eq 'to_device') {
				$retVal = $retVal + $offset;
				$retVal = int($retVal * $factor); 
			} else {
#				$retVal = $retVal / $factor;
				$retVal = sprintf("%.2f", $retVal / $factor);
				$retVal = $retVal - $offset;
			}
#print Dumper($retVal, $factor, $t);

		} elsif ($type eq 'boolean_integer') {
			my $threshold = $convertConfig->{threshold} ? $convertConfig->{threshold} : 1;
			my $invert    = $convertConfig->{invert} ? 1 : 0;
			my $false     = $convertConfig->{false} ? $convertConfig->{false} : 0;
			my $true      = $convertConfig->{true} ? $convertConfig->{true} : 1;

			if ($dir eq 'to_device') {
				$retVal = ($retVal >= $threshold) ? 1 : 0;
				$retVal = (($invert && $retVal) || (!$invert && !$retVal)) ? 0 : 1; 
			} else {
				$retVal = (($invert && $retVal) || (!$invert && !$retVal)) ? 0 : 1; 
				$retVal = ($retVal >= $threshold) ? $true : $false;
			}

		# Todo float_configtime from 
		#} elsif ($config eq 'float_configtime') {
		#	$valueMap = 'IntInt';

		#} elsif ($config eq 'option_integer') {
		#	$valueMap = 'value';

		}
	}
	
	return $retVal;
}

sub getChannelValueMap($$$$) {
	my ($hash, $deviceKey, $frameData, $valId) = @_;
	
	my $channel = $frameData->{ch};
	my $chType = getChannelType($deviceKey, $channel);

	my $hmwId = $hash->{DEF}; 
	my $chHash = $main::modules{HM485}{defptr}{$hmwId . '_' . $channel};

	my $values;
	my $channelBehaviour = HM485::Device::getChannelBehaviour($chHash);

# Todo: Check $channelBehaviour and $valuePrafix
#	my $valuePrafix = $channelBehaviour ? '.' . $channelBehaviour : '';
	my $valuePrafix = '';

	$values  = getValueFromDefinitions(
		$deviceKey . '/channels/' . $chType .'/params/values' . $valuePrafix . '/'
	);

	my $retVal;
	if (defined($values)) {
		foreach my $value (keys %{$values}) {
			if ($values->{$value}{physical}{value_id} eq $valId) {
				if (!defined($values->{$value}{physical}{event}{frame}) ||
					$values->{$value}{physical}{event}{frame} eq $frameData->{id}
				) {
					$retVal = $values->{$value};
					$retVal->{name} = $value;
					last;
				}
			}
		}
	}
	
	return $retVal;
}

sub getEmptyEEpromMap ($) {
	my ($hash) = @_;

	my $deviceKey = HM485::Device::getDeviceKeyFromHash($hash);
	my $eepromAddrs = parseForEepromData(getValueFromDefinitions($deviceKey));

	my $eepromMap = {};
	my $blockLen = 16;
	my $blockCount = 0;
	my $addrMax = 1024;
	my $adrCount = 0;
	my $hexBlock;

	for ($blockCount = 0; $blockCount < ($addrMax / $blockLen); $blockCount++) {
		my $blockStart = $blockCount * $blockLen;
		foreach my $adrStart (sort keys %{$eepromAddrs}) {
			my $len = $adrStart + $eepromAddrs->{$adrStart};
			if (($adrStart >= $blockStart && $adrStart < ($blockStart + $blockLen)) ||
			    ($len >= $blockStart)
			   ) {

				my $blockId = sprintf ('%04X' , $blockStart);
				if (!$eepromMap->{$blockId}) {
					$eepromMap->{$blockId} = '##' x $blockLen;
				}
				if ($len <= ($blockStart + $blockLen)) {
					delete ($eepromAddrs->{$adrStart});				
				}
			} else {
				last;
			}
		}
	}

	return $eepromMap;
}

=head2
	Get EEprom data from hash->READINGS with specific start address and lenth

	@param	hash       hash	hash of device addressed
	@param	int        start address
	@param	int        count bytes to retreve
	@param	boolean    if 1 return as hext string
	
	@return string     value string
=cut
sub getRawEEpromData($;$$$$) {
	my ($hash, $start, $len, $hex, $littleEndian) = @_;

	my $hmwId   = $hash->{DEF};
	my $devHash = $main::modules{HM485}{defptr}{substr($hmwId,0,8)};

	my $blockLen = 16;
	my $addrMax = 1024;
	my $blockStart = 0;
	my $blockCount = 0;
	
	$start        = defined($start) ? $start : 0;
	$len          = defined($len) ? $len : $addrMax;
	$hex          = defined($hex) ? $hex : 0;
	$littleEndian = defined($littleEndian) ? $littleEndian : 0;

	if ($start > 0) {
		$blockStart = int($start/$blockLen);
	}

	my $retVal = '';
	for ($blockCount = $blockStart; $blockCount < (ceil($addrMax / $blockLen)); $blockCount++) {
		my $blockId = sprintf ('.eeprom_%04X' , ($blockCount * $blockLen));
		if ($devHash->{READINGS}{$blockId}{VAL}) {
			$retVal.= $devHash->{READINGS}{$blockId}{VAL};
		} else {
			$retVal = 'FF' x $blockLen;
		}

		if (length($retVal) / 2 >= $len) {
			last;
		}
	}

	my $start2 = ( ( ($start/$blockLen) - $blockStart ) * $blockLen );
	$retVal = pack('H*', substr($retVal, ($start2 * 2), ($len * 2) ) );
	
	$retVal = $littleEndian ? reverse($retVal) : $retVal;
	$retVal = $hex ? unpack('H*', $retVal) : $retVal;

#print Dumper("+++++++ \n", unpack ('H*',$retVal), $start, $len, "\n -------");		
	
	return $retVal;
}

sub setRawEEpromData($$$$) {
	my ($hash, $start, $len, $data) = @_;

	$data = substr($data, 0, ($len*2));
	$len = length($data);
	my $blockLen = 16;
	my $addrMax = 1024;
	my $blockStart = 0;
	my $blockCount = 0;
	
	if (hex($start) > 0) {
		$blockStart = int((hex($start) * 2) / ($blockLen*2));
	}

	for ($blockCount = $blockStart; $blockCount < (ceil($addrMax / $blockLen)); $blockCount++) {

		my $blockId = sprintf ('.eeprom_%04X' , ($blockCount * $blockLen));
		my $blockData = $hash->{READINGS}{$blockId}{VAL};
		if (!$blockData) {
			# no blockdata defined yet
			$blockData = 'FF' x $blockLen;
		}

		my $dataStart = (hex($start) * 2) - ($blockCount * ($blockLen * 2));
		my $dataLen = $len;

		if ($dataLen > (($blockLen * 2) - $dataStart)) {
			$dataLen = ($blockLen * 2) - $dataStart;
		}

		my $newBlockData = $blockData;

		if ($dataStart > 0) {
			$newBlockData = substr($newBlockData, 0, $dataStart);
		} else {
			$newBlockData = '';
		}

		$dataLen = ($len <= $dataLen) ? $len : $dataLen;
		$newBlockData.= substr($data, 0, $dataLen);

		if ($dataStart + $dataLen < ($blockLen * 2)) {
			$newBlockData.= substr(
				$blockData, ($dataStart + $dataLen), ($blockLen * 2) - $dataStart + $dataLen
			);
			$data = '';
		} else {
			$data = substr($data, $dataLen);
			$start = ($blockCount * $blockLen) + $blockLen;
		}
		
		$hash->{READINGS}{$blockId}{VAL} = $newBlockData;

		$len = length($data);
		if ($len == 0) {
			last;
		}
	}
}

=head2
	Walk thru device definition and found all eeprom related values
	
	@param	hash    the whole config for thie device
	@param	hash    holds the the eeprom adresses with length
	@param	hash    spechial params passed while recursion for getEEpromData
	
	@return hash    $adrHash
=cut
sub parseForEepromData($;$$) {
	my ($configHash, $adrHash, $params) = @_;

	$adrHash = $adrHash ? $adrHash : {};
	$params  = $params ? $params : {};
	
	# first we must collect all values only, hahes was pushed to hash array
	my @hashArray = ();
	foreach my $param (keys %{$configHash}) {
		if (ref($configHash->{$param}) ne 'HASH') {
			if ($param eq 'count' || $param eq 'address_start' || $param eq 'address_step') {
				$params->{$param} = $configHash->{$param};
			}
		} else {
			push (@hashArray, $param);
		}
	}

	# now we parse the hashes
	foreach my $param (@hashArray) {
		my $p = $configHash->{$param};
		if ($p->{physical} && $p->{physical}{interface} && $p->{physical}{interface} eq 'eeprom') {
			my $result = getEEpromData($p, $params);
			@{$adrHash}{keys %$result} = values %$result;
		} else {
			$adrHash = parseForEepromData($p, $adrHash, {%$params});
		}
	}
	
	return $adrHash;
}

=head2
	calculate the eeprom adress with length for a specific param hash
	
	@param	hash    the param hash
	@param	hash    spechial params passed while recursion for getEEpromData

	@return hash    eeprom addr -> length
=cut
sub getEEpromData($$) {
	my ($paramHash, $params) = @_;
	
	my $count = ($params->{count} && $params->{count} > 0) ? $params->{count} : 1; 
	my $retVal;
	
	if ($params->{address_start} && $params->{address_step}) {
		my $adrStart  = $params->{address_start} ? $params->{address_start} : 0; 
		my $adrStep   = $params->{address_step} ? $params->{address_step} : 1; 
		$adrStart = sprintf ('%04d' , $adrStart);
		$retVal->{$adrStart} = $adrStep * $count;

	} else {
		if ($paramHash->{physical}{address_id}) {
			my $adrStart =  $paramHash->{physical}{address_id};
			$adrStart = sprintf ('%04d' , $adrStart);

			my $size = $paramHash->{physical}{size};
			$size = $size * $count;
			$size = isInt($paramHash->{physical}{size}) ? $size : ceil(($size / 0.8));

			$retVal->{$adrStart} = $size;
		}
	}

	return $retVal;
}

sub getChannelsByModelgroup ($) {
	my ($deviceKey) = @_;
	my $channels = getValueFromDefinitions($deviceKey . '/channels/');
	my @retVal = ();
	foreach my $channel (keys %{$channels}) {
		push (@retVal, $channel);
	}
	
	return @retVal;
}

sub isNumber($) {
	my ($value) = @_;
	
	my $retVal = (looks_like_number($value)) ? 1 : 0;
	
	return $retVal;
}

sub isInt($) {
	my ($value) = @_;
	$value = (looks_like_number($value)) ? $value : 0;
	
	my $retVal = ($value == int($value)) ? 1 : 0;
	return $retVal;
}

sub subBit ($$$) {
	my ($byte, $start, $len) = @_;
	
	return (($byte << (8 - $start - $len)) & 0xFF) >> (8 - $len);
}









sub internalUpdateEEpromData($$) {
	my ($devHash, $requestData) = @_;

	my $start = substr($requestData, 0,4);
	my $len   = substr($requestData, 4,2);
	my $data  = substr($requestData, 6);

	setRawEEpromData($devHash, $start, $len, $data);
}

sub parseModuleType($) {
	my ($data) = @_;
	
	my $modelNr = hex(substr($data,0,2));
	my $retVal   = getModelFromType($modelNr);
	$retVal =~ s/-/_/g;
	
	return $retVal;
}

sub parseSerialNumber($) {
	my ($data) = @_;
	
	my $retVal = substr(pack('H*',$data), 0, 10);
	
	return $retVal;
}

sub parseFirmwareVersion($) {
	my ($data) = @_;
	my $retVal = undef;
	
	if (length($data) == 4) {
		$retVal = hex(substr($data,0,2));
		$retVal = $retVal + (hex(substr($data,2,2))/100);
	}

	return $retVal;
}

sub getAllowedSets($) {
	my ($hash) = @_;

	my $name   = $hash->{NAME};
	my $model  = $hash->{MODEL};
	my $onOff  = 'on:noArg off:noArg ';
	my $keys   = 'press_short:noArg press_long:noArg';

	my $retVal = undef;
	if (defined($model) && $model) {
		
		my ($hmwId, $chNr) = HM485::Util::getHmwIdAndChNrFromHash($hash);

		if (defined($chNr)) {
			my $channelBehaviour = getChannelBehaviour($hash);
			if ($channelBehaviour) {
				$hash->{behaviour} = $channelBehaviour;
				if ($channelBehaviour eq 'output' || $channelBehaviour eq 'digital_output') {
					$retVal = $onOff;

				} elsif ($channelBehaviour eq 'analog_output') {
					$retVal = 'frequency:textField';

				} elsif ($channelBehaviour eq 'input' || $channelBehaviour eq 'digital_input') {
					$retVal = $keys;

				} elsif ($channelBehaviour eq 'frequency_input') {
					$retVal = 'frequency:slider,0,1,100 frequency2:textField';
				}
				
			} else {
				my $deviceKey = HM485::Device::getDeviceKeyFromHash($hash);
				my $chType    = getChannelType($deviceKey, $chNr);

				if ($chType eq 'key') {
					$retVal = $keys;
		
				} elsif ($chType eq 'switch' || $chType eq 'digital_output') {
					$retVal = $onOff;
	
				} elsif ($chType eq 'dimmer' || $chType eq 'blind') {
					$retVal = $onOff . 'level:slider,0,1,100';
				}
			}
		}
	}

	return $retVal;
}

1;