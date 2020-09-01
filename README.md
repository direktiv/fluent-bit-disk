[![Codacy Badge](https://app.codacy.com/project/badge/Grade/1f419c28498848e5b8bbb6f3c0f1b0ef)](https://www.codacy.com/gh/vorteil/fluent-bit-disk?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=vorteil/fluent-bit-disk&amp;utm_campaign=Badge_Grade) [![Build Status](https://travis-ci.org/vorteil/fluent-bit-disk.svg?branch=master)](https://travis-ci.org/vorteil/fluent-bit-disk)

# fluent-bit-disk

## Using the plugin

This plugin for [fluent-bit](https://github.com/fluent/fluent-bit) reports disk partition usage.

The name of the plugin is 'vdisk' and the name of the partition can be defined in the INPUT
section of the plugin.

### Example configuration

```sh
[SERVICE]
        Daemon off
        Flush 1

[INPUT]
        Name vdisk
        Partition /run

[OUTPUT]
        Name stdout
```

To make the plugin available to fluentbit the path of the share object needs to be specified on the command line:

```sh
./fluent-bit --config=myconfig.cfg --plugin=/path/to/flb-in_vdisk.so
```

This will produce output like the following:

```sh
[0] vdisk.0: [1598916699.022796699, {"bytes_total"=>3361153024, "bytes_free"=>3358642176, "bytes_pct"=>0.074702, "inodes_total"=>4101586, "inodes_free"=>4102966, "inodes_pct"=>0.033634}]
```

## Building the plugin

This plugin is using the example plugin setup for [fluentbit plugins](https://github.com/fluent/fluent-bit-plugin).

```sh
cmake -DPLUGIN_NAME=in_vdisk -DFLB_SOURCE=~/path/to/fluentbit  ..
```
