package HM485::Devicefile;

our %definition = (
	'HBW_Sen_EP'	=>{
		'version'		=> 12,
		'eeprom-size'	=> 1024,
		'models'	=> {
			'HBW_Sen_EP'	=> {
				'name'	=> 'Homebrew wired S0-Interface',
				'type'	=> 132,
			},
		},
		'params' => {
			'master'	=> {
				'central_address'	=> {
					'hidden'		=> 1,
					'enforce'		=> 0x00000001,
					'logical'		=> {
						'type'		=> 'int',
					},
					'physical'			=> {
						'type'			=> 'int',
						'size'			=> 4,
						'interface'		=> 'eeprom',
						'address_id'	=> 0x0002
					}
				}
			}
		},
		'frames'	=> {
			'level_get'	=> {
				'type'		=> 0x53,
				'dir'		=> 'to_device', 
				'ch_field'	=> 10,
			},
			'info_level'	=> {
				'type'		=> 0x69,
				'dir'		=> 'from_device',
				'event'		=> 1,
				'ch_field'	=> 10,
				'params'	=> {
					'state'		=> {
						'type'	=> 'int',
						'id'	=> 11.0,
						'size'	=> 2
					}
				}
			},
			'info_frequency'	=> {
				'type'		=> 0x69,
				'dir'		=> 'from_device',
				'event'		=> 1,
				'ch_field'	=> 10,
				'params'	=> {
					'state'		=> {
						'type'	=> 'int',
						'id'	=> 11.0,
						'size'	=> 3
					}
				}
			}
		},
		'channels'	=> {
			'maintenance'	=> {
				'id'		=> 0,
				'ui-flags'	=> 'internal',
				'class'		=> 'maintenance',
				'count'	=> 1,
				'params'	=> {
					'master'	=> {},
					'values'	=> {
						'unreach'	=> {
							'operations'	=> 'read,event',
							'ui-flags'		=> 'service',
							'logical'		=> {
								'type'		=> 'boolean',
							},
							'physical'		=> {
								'type'		=> 'int',
								'interface'	=> 'internal',
							},
						},
						'sticky_unreach'	=> {
							'operations'	=> 'read,write,event',
							'ui-flags'		=> 'service',
							'logical'		=> {
								'type'		=> 'boolean',
							},
							'physical'		=> {
								'type'		=> 'int',
								'interface'	=> 'internal',
							}
						},
						'config_pending'	=> {
							'operations'	=> 'read,event',
							'ui-flags'		=> 'service',
							'logical'		=> {
								'type'		=> 'boolean',
							},
							'physical'		=> {
								'type'		=> 'int',
								'interface'	=> 'internal',
							}
						}
					}
				}
			},
			'counter_input'	=> {
				'id'	=> 1,
				'count'	=> 8,
				'physical_id_offset'	=> -1,
				'params'	=> {
					'master'	=> {
						'address_start'	=> 0x07,
						'address_step'	=> 6,
						'send_delta_count'	=> {
							'logical'	=> {
								'type'		=> 'integer',
								'min' 		=> 1,
								'max'	 	=> 1000,
								'default'	=> 1,
							},
							'physical'	=> {
								'type'			=> 'int',
								'size'			=> 2,
								'interface'		=> 'eeprom',
								'endian'		=> 'little',
								'address_id'	=> 0.0
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
								'type'			=> 'int',
								'size'			=> 2,
								'interface'		=> 'eeprom',
								'endian'		=> 'little',
								'address_id'	=> 2.0
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
								'type'			=> 'int',
								'size'			=> 2,
								'interface'		=> 'eeprom',
								'endian'		=> 'little',
								'address_id'	=> 4.0
							}
						}
					},
					'values'	=> {
						'counter' => {
							'operations'	=> 'read,event', 
							'logical'		=> {
								'type'		=> 'int',
								'min'		=> 0,
								'max'		=> 65535,
							},
							'physical'		=> {
								'type'		=> 'int',
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
);

1;