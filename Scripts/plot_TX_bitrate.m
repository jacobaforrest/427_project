path = "Results/TX_bitrate.dat";
[folder, filename, ~] = fileparts(path);
% Read in data from file
data = importdata(path);

% Separate x and y data into separate vectors
x = data(:,1);
y = data(:,2);

% Create a new figure and plot the data
figure();
stem(x, y, '-o', 'MarkerSize', 6);

% Label the axes and give the plot a title
xlabel('Number of Attack Bots');
%xticks([0, 4, 8, 10, 12, 16]);
ylabel('Bitrate (kbps)');
title('Transmit Bitrate vs. Number of Attack Bots');
fontsize(gca,24,"pixels")
set(gcf, 'Position', [100 100 800 600]);
set(gcf, 'Visible', 'off');
print(folder + '/' + filename + '.png', '-dpng', '-r600');
