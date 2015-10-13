package HM485::Devicefile;
our %definition = (
	'HBW_LC_DIM99' => {
		'version' => 	11,
		'eep_size' => 	1024,
		'supported_types' => 	{
			"HBW_LC_Dim99" => {
				"name" => "RS485 dimming actuator 99-channel)",
				"parameter" => {
					0 => {
						"const_value" => 146,
						"size" => 1
					},
					1 => {
						"const_value" => 0,
						"size" => 1
					},
					2 => {
						"cond_op" => "GE",
						"const_value" => 0x0100,
						"size" => 2
					}
				},
				"priority" => 2
			}
		},
		'paramset' => 	{
			"enforce" => {
				"central_address" => {
					"value" => 1
				},
				"direct_link_deactivate" => {
					"value" => true
				}
			},
			"id" => "hbw-lc-dim99_dev_master",
			"parameter" => {
				"central_address" => {
					"hidden" => true,
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
				"direct_link_deactivate" => {
					"hidden" => true,
					"logical" => {
						"default" => false,
						"type" => "boolean"
					},
					"physical" => {
						"address" => {
							"index" => 0x0006
						},
						"interface" => "eeprom",
						"size" => 0.1,
						"type" => "integer"
					}
				},
				"logging_time" => {
					"conversion" => {
						"factor" => 10,
						"offset" => 0.0,
						"type" => "float_integer_scale"
					},
					"logical" => {
						"default" => 2.0,
						"max" => 25.5,
						"min" => 0.1,
						"type" => "float",
						"unit" => "s"
					},
					"physical" => {
						"address" => {
							"index" => 0x0001
						},
						"interface" => "eeprom",
						"size" => 1.0,
						"type" => "integer"
					}
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
					"11.0" => {
						"param" => "level",
						"size" => 1.0,
						"type" => "integer"
					},
					"12.4" => {
						"param" => "state_flags",
						"size" => 0.3,
						"type" => "integer"
					}
				},
				"type" => 0x69
			},
			"level_get" => {
				"channel_field" => 10,
				"direction" => "to_device",
				"type" => 0x53
			},
			"level_set" => {
				"channel_field" => 10,
				"direction" => "to_device",
				"parameter" => {
					"index" => 11.0,
					"param" => "level",
					"size" => 1.0,
					"type" => "integer"
				},
				"type" => 0x78
			}
		},
		'channels' => 	{
			"dimmer" => {
				"count" => 99,
				"index" => 1,
				"paramset" => {
					"values" => {
						"parameter" => {
							"direction" => {
								"conversion" => {
									"type" => "option_integer",
									"value_map" => {
										"1" => {
											"device_value" => 0x00,
											"parameter_value" => 0
										},
										"2" => {
											"device_value" => 0x01,
											"parameter_value" => 1
										},
										"3" => {
											"device_value" => 0x02,
											"parameter_value" => 2
										},
										"4" => {
											"device_value" => 0x03,
											"parameter_value" => 3
										}
									}
								},
								"logical" => {
									"option" => {
										"down" => {},
										"none" => {
											"default" => true
										},
										"undefined" => {},
										"up" => {}
									},
									"type" => "option"
								},
								"operations" => "read,event",
								"physical" => {
									"event" => {
										"1" => {
											"frame" => "info_level"
										},
										"2" => {
											"frame" => "ack_status"
										}
									},
									"get" => {
										"request" => "level_get",
										"response" => "info_level"
									},
									"interface" => "command",
									"type" => "integer",
									"value_id" => "state_flags"
								},
								"ui_flags" => "internal"
							},
							"level" => {
								"control" => "blind.level",
								"conversion" => {
									"factor" => 2,
									"type" => "float_integer_scale"
								},
								"logical" => {
									"default" => 0,
									"max" => 100,
									"min" => 0,
									"type" => "int",
									"unit" => "%"
								},
								"operations" => "read,write,event",
								"physical" => {
									"event" => {
										"frame" => "info_level"
									},
									"get" => {
										"request" => "level_get",
										"response" => "info_level"
									},
									"interface" => "command",
									"set" => {
										"request" => "level_set"
									},
									"type" => "integer",
									"value_id" => "level"
								}
							},
						},
						"type" => "values"
					}
				},
				"physical_index_offset" => -1
			},
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
			}
		},
	}
);	
