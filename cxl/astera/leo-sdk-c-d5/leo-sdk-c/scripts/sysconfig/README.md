## Usage

### *Edit config*
```sh
python3 sysconfig_manager.py <FW image binary> -cfg <config file>
```

Arguments:
```
positional arguments:
  fw          Full path to FW image

optional arguments:
  -h, --help  show this help message and exit
  -cfg CFG    Full path to pre-boot options config file
  -out [OUT]  Output .mem binary name, or auto-generate a name if no argument is provided
  -dump       Dump values of configured options on target flash binary
```

Recommended usage for quick changes: 
```sh
python3 sysconfig_manager.py <FW image binary> -cfg <config file> -out
```

Recommended usage for automation
```
python3 sysconfig_manager.py <FW image binary> -cfg <config file> -out <identifier (date, time)>
```

### *Dump config*
```sh
python3 sysconfig_manager.py <FW image binary> -dump
```

## Template: sysconfig.json

This template config file contains all sysconfig fields that are available to be configured pre-boot. Each field is set to null by default, which means that field will not be configured. 

Some fields are enabled with **true**, and some require a specific value to be provided. For example, **"speed"** requires a frequency in MT/s and temp_threshold values require a temperature in Degrees Celcius.

```json
{
    "cxl2x8": null,
    "ddr2dpc": null,
    "compliance": null,
    "ddrinterleaving": null,
    "speed": null,
    "alti2c": null,
    "pcie": null,
    "aes": null,
    "sbrefresh": null,
    "temp_threshold": {
      "0": null,
      "1": null,
      "2": null,
      "c": null
    },
    "bw_throttle": {
      "0": null,
      "1": null,
      "2": null
    },
    "temp_sample_interval_s": null,
    "ddr_page_close": null,
    "cxl_temp_threshold": {
      "warning_lo": null,
      "warning_hi": null,
      "critical_lo": null,
      "critical_hi": null
    },
    "cxl_correctable_err_threshold": null,
    "cxl_mem_size": null
}
```


## valid values for each field

*providing a value of null indicates that an option will not be configured*

### cxl2x8, ddr2dpc, compliance, alti2c, pcie, aes, sbrefresh, ddr_page_close
- These features are either enabled or disabled. Valid values are [`true`, `null`]

### speed
  *Units are MT/s*
  ```json
  {
    "speed": null,
    "speed": 3200,
    "speed": 3600,
    "speed": 4000,
    "speed": 4400,
    "speed": 4800,
    "speed": 5200,
    "speed": 5600,
    "speed": 6000,
    "speed": 6400,
    "speed": 6800,
    "speed": 7200,
    "speed": 7600,
    "speed": 8000,
    "speed": 8400,
    "speed": 1600,
    "speed": 2100,
  }
  ```

### ddrinterleaving

  *Units are interleaving ways.*
  Using a value `true` is equivalent to using default value `4`
  
  ```json
  {
    "ddrinterleaving": null,
    "ddrinterleaving": true,
    "ddrinterleaving": 2,
    "ddrinterleaving": 4,
  }
  ```

### temp_threshold

*Units are Degrees Celcius*

Not all values must be configured, but configured values must map to the corresponding bw_threshold value.
The range of valid values is [0, 127]. The firmware will set a default value for "c" (Cattrip) if not configured.

**Example**, corresponding with the example in bw_throttle:

  ```json
  {
    "temp_threshold": {
      "0": null,
      "1": 65,
      "2": 85,
      "c": null
    },
  }
  ```

### bw_threshold

*Units are % of bandwidth to make available*

Not all values must be configured, but configured values must map to the corresponding temp_threshold value.

The valid values are {10, 25, 50, 100}.
  
 ```json
  {
    "bw_throttle": {
      "0": null,
      "1": 25,
      "2": 10,
    },
  }
  ```
  
### temp_sample_interval_s

*Units are Seconds*

Range of valid values are [1, 127]

```json
  {
    "temp_sample_interval_s": null,
    "temp_sample_interval_s": 5,
    "temp_sample_interval_s": 90,
  }
  ```


### cxl_temp_threshold

  *Units are Degrees Celcius*
  
  The range of valid values is [0, 127]. 

  Example:
```json
  {
    "cxl_temp_threshold": {
      "warning_lo": 61,
      "warning_hi": 69,
      "critical_lo": 85,
      "critical_hi": 89
    },
  }
```


### cxl_correctable_err_threshold

*Units are Correctable Error Count*

Range of valid values are [0, 65535]

```json
  {
    "cxl_correctable_err_threshold": null,
    "cxl_correctable_err_threshold": 65535,
  }
```

### cxl_mem_size

```json
  {
    "cxl_mem_size": null,
    "cxl_mem_size": 16,
    "cxl_mem_size": 32,
    "cxl_mem_size": 64,
  }
```
