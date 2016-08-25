#!/usr/bin/perl

use strict;
use warnings;
no warnings qw(uninitialized);

my ($desired_speed) = @ARGV; # mph
my $desired_speed_metric = $desired_speed * 0.44704; # convert from MPH to m/s
my $wheel_diameter = 0.127; # meters
my $wheel_circumference = $wheel_diameter * 3.14159265;
my $n_revolutions = $desired_speed_metric / $wheel_circumference;
my $n_clicks = $n_revolutions * 40;

printf "Desired speed: %f mph -> %f m/s\n", $desired_speed, $desired_speed_metric;
printf "RPM: %f\n", $n_revolutions * 60;
printf "Set point: %d\n", $n_clicks;

exit;
