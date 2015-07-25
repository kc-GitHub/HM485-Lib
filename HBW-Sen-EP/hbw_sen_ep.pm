package HM485::Devicefile;

our %definition = (
    'HBW_Sen_EP'	=>{
	'version'		=> 12,
	'eep-size'	=> 1024,
	'supported_types' => 	{
	    'HBW_Sen_EP'	=> {
		'name'	=> 'Homebrew wired S0-Interface',
		"parameter" => {
		    "0" => {
			"const_value" => 132,
			"size" => 1
		    },
		    "1" => {
			"const_value" => 0,
			"size" => 1
		    },
		    "2" => {
			"cond_op" => "GE",
			"const_value" => 0x0100,
			"size" => 2
		    }
		},
		"priority" => 2
	    },
	},
	'paramset' => {
	    "enforce" => {
		"central_address" => {
		    "value" => 1
		},
	    },
	    "id" => "HBW_Sen_EP_dev_master",
	    'parameter'	=> {
		'central_address'	=> {
		    'hidden'		=> true,
		    'logical'		=> {
			'type'		=> 'integer',
		    },
		    'physical'			=> {
			"address" => {
			    "index" => 0x0002
			},
			"interface" => "eeprom",
			"size" => 4,
			"type" => "integer"
		    }
		}
	    },
	    "type" => "master"
	},
	'frames'	=> {
	    'level_get'	=> {
		'type'		=> 0x53,
		'direction'		=> 'to_device', 
		'channel_field'	=> 10,
	    },
	    'info_level'	=> {
		'type'		=> 0x69,
		'direction'		=> 'from_device',
		'event'		=> true,
		'channel_field'	=> 10,
		'parameter'	=> {
		    "11.0" => {
			"param" => 'state',
			'type'	=> 'integer',
			'size'	=> 2.0
		    },
		}
	    },
	    'info_frequency'	=> {
		'type'		=> 0x69,
		'direction'		=> 'from_device',
		'event'		=> true,
		'channel_field'	=> 10,
		'parameter'	=> {
		    "11.0" => {
			"param" => 'state',
			'type'	=> 'integer',
			'size'	=> 3.0
		    },
		}
	    }
	},
	'channels'	=> {
	    'maintenance'	=> {
		'index'		=> 0,
		'ui-flags'	=> 'internal',
		'class'		=> 'maintenance',
		'count'	=> 1,
		"paramset" => {
		    "maint_ch_master" => {
			"type" => "master"
		    },
		    "maint_ch_values" => {
			"parameter" => {
			    "config_pending" => {
				"logical" => {
				    "type" => "boolean"
				},
				"operations" => "read,event",
				"physical" => {
				    "interface" => "internal",
				    "type" => "integer",
				    "value_id" => "config_pending"
				},
				"ui_flags" => "service"
			    },
			    "sticky_unreach" => {
				"logical" => {
				    "type" => "boolean"
				},
				"operations" => "read,write,event",
				"physical" => {
				    "interface" => "internal",
				    "type" => "integer",
				    "value_id" => "sticky_unreach"
				},
				"ui_flags" => "service"
			    },
			    "unreach" => {
				"logical" => {
				    "type" => "boolean"
				},
				"operations" => "read,event",
				"physical" => {
				    "interface" => "internal",
				    "type" => "integer",
				    "value_id" => "unreach"
				},
				"ui_flags" => "service"
			    }
			},
			"type" => "values"
		    }
		}
	    },
	    'counter_input'	=> {
		'index'	=> 1,
		'count'	=> 8,
		'physical_index_offset'	=> -1,
		'paramset'	=> {
		    'master'	=> {
			'address_start'	=> 0x07,
			'address_step'	=> 6,
			"parameter" => {
			'send_delta_count'	=> {
			    'logical'	=> {
				'type'		=> 'integer',
				'min' 		=> 1,
				'max'	 	=> 1000,
				'default'	=> 1,
			    },
			    'physical'	=> {
				'type'			=> 'integer',
				'size'			=> 2.0,
				'interface'		=> 'eeprom',
				'endian'		=> 'little',
				"address" => {
				    "index" => 0.0
				},
			    }
			},
			'send_min_interval'	=> {
			    'logical'	=> {
				'type'		=> 'integer',
				'min' 		=> 0,
				'max'	 	=> 3600,
				'default'	=> 10,
				'unit'		=> 's',
			    },
			    'physical'	=> {
				'type'			=> 'integer',
				'size'			=> 2.0,
				'interface'		=> 'eeprom',
				'endian'		=> 'little',
				"address" => {
					"index" => 2.0
				},
			    }
			},
			'send_max_interval'	=> {
			    'logical'	=> {
				'type'		=> 'integer',
				'min' 		=> 5,
				'max'	 	=> 3600,
				'default'	=> 600,
				'unit'		=> 's',
			    },
			    'physical'	=> {
				'type'			=> 'integer',
				'size'			=> 2.0,
				'interface'		=> 'eeprom',
				'endian'		=> 'little',
				"address" => {
				    "index" => 4.0
				},
			    }
			}
			}
		    },
		    'values'	=> {
			"type" => "values",
			"parameter" => {
			'counter' => {
			    'operations'	=> 'read,event', 
			    'control'		=> 'none',
			    'logical'		=> {
				'type'		=> 'integer',
				'min'		=> 0,
				'max'		=> 65535,
			    },
			    'physical'		=> {
				'type'		=> 'integer',
				'interface'	=> 'command',
				'value_id'	=> 'state',
				'get'		=> {
				    'request'	=> 'level_get',
				    'response'	=> 'info_level'
				},
				'event'		=> {
				    'frame'	=> 'info_level'
				}
			    },
			    'conversion'	=> {
				'type'		=> 'float_integer_scale',
				'factor'	=> 1
			    }
			}
			}
		    }
		}
	    }
	}
    }
);

1;