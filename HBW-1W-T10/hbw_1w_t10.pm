package HM485::Devicefile;
our %definition = (
	'HBW_1W_T10' => {
		'version' => 	12,
		'eep_size' => 	1024,
		'supported_types' => 	{
			"HBW_1W_T10" => {
				"name" => "RS485 One Wire 10 Channel Temperature Module",
				"parameter" => {
					0 => {
						"const_value" => 129,
						"size" => 1
					},
					1 => {
						"const_value" => 0,
						"size" => 1
					}
				},
				"priority" => 2
			}
		},
		'paramset' => 	{
			"enforce" => {
				"id" => "central_address",
				"value" => 1
			},
			"id" => "hbw-1w-t10_dev_master",
			"parameter" => {
				"hidden" => true,
				"id" => "central_address",
				"logical" => {
					"type" => "integer"
				},
				"physical" => {
					"address" => {
						"index" => 0x0002
					},
					"interface" => "eeprom",
					"size" => 4,
					"type" => "integer"
				}
			},
			"type" => "master"
		},
		'frames' => 	{
			"info_level" => {
				"channel_field" => 10,
				"direction" => "from_device",
				"event" => true,
				"parameter" => {
					"index" => 11.0,
					"param" => "temperature",
					"signed" => true,
					"size" => 2.0,
					"type" => "integer"
				},
				"type" => 0x69
			},
			"level_get" => {
				"channel_field" => 10,
				"direction" => "to_device",
				"type" => 0x53
			}
		},
		'channels' => 	{
			"maintenance" => {
				"class" => "maintenance",
				"count" => 1,
				"index" => 0,
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
				},
				"ui_flags" => "internal"
			},
			"tempsensor" => {
				"count" => 10,
				"index" => 1,
				"paramset" => {
					"link" => {},
					"master" => {
						"address_start" => 0x10,
						"address_step" => 16,
						"parameter" => {
							"onewire_type" => {
								"logical" => {
									"max" => 254,
									"min" => 0,
									"special_value" => {
										"id" => "not_used",
										"value" => 255
									},
									"type" => "integer"
								},
								"physical" => {
									"address" => {
										"index" => 7.0
									},
									"interface" => "eeprom",
									"size" => 1.0,
									"type" => "integer"
								}
							},
							"send_delta_temp" => {
								"conversion" => {
									1 => {
										"factor" => 10,
										"type" => "float_integer_scale"
									},
									2 => {
										"type" => "integer_integer_map",
										"value_map" => {
											"device_value" => 0xFF,
											"from_device" => true,
											"parameter_value" => 5,
											"to_device" => false
										}
									}
								},
								"logical" => {
									"default" => 0.5,
									"max" => 25.0,
									"min" => 0.1,
									"special_value" => {
										"id" => "not_used",
										"value" => 0
									},
									"type" => "float",
									"unit" => "°C"
								},
								"physical" => {
									"address" => {
										"index" => 1.0
									},
									"interface" => "eeprom",
									"size" => 1.0,
									"type" => "integer"
								}
							},
							"send_max_interval" => {
								"conversion" => {
									"type" => "integer_integer_map",
									"value_map" => {
										"device_value" => 0xFFFF,
										"from_device" => true,
										"parameter_value" => 150,
										"to_device" => false
									}
								},
								"logical" => {
									"default" => 150,
									"max" => 3600,
									"min" => 5,
									"special_value" => {
										"id" => "not_used",
										"value" => 0
									},
									"type" => "integer",
									"unit" => "s"
								},
								"physical" => {
									"address" => {
										"index" => 5.0
									},
									"endian" => "little",
									"interface" => "eeprom",
									"size" => 2.0,
									"type" => "integer"
								}
							},
							"send_min_interval" => {
								"conversion" => {
									"type" => "integer_integer_map",
									"value_map" => {
										"device_value" => 0xFFFF,
										"from_device" => true,
										"parameter_value" => 10,
										"to_device" => false
									}
								},
								"logical" => {
									"default" => 10,
									"max" => 3600,
									"min" => 5,
									"special_value" => {
										"id" => "not_used",
										"value" => 0
									},
									"type" => "integer",
									"unit" => "s"
								},
								"physical" => {
									"address" => {
										"index" => 3.0
									},
									"endian" => "little",
									"interface" => "eeprom",
									"size" => 2.0,
									"type" => "integer"
								}
							}
						},
						"type" => "master"
					},
					"values" => {
						"parameter" => {
                                                   "temperature" => {
							"conversion" => {
								"factor" => 100,
								"type" => "float_integer_scale"
							},
							"id" => "temperature",
							"logical" => {
								"max" => 327.67,
								"min" => "-273.15",
								"type" => "float",
								"unit" => "°C"
							},
							"operations" => "read,event",
							"physical" => {
								"event" => {
									"frame" => "info_level"
								},
								"get" => {
									"request" => "level_get",
									"response" => "info_level"
								},
								"interface" => "command",
								"type" => "integer",
								"value_id" => "temperature"
							}
                                                    }
						},
						"type" => "values"
					}
				},
				"physical_index_offset" => -1
			}
		},
	}
);	
