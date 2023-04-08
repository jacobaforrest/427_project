function plot_tcp_congestion(path, num_bots)

    [folder, filename, ~] = fileparts(path);
    % Read in data from file
    data = importdata(path);
    
    % Separate x and y data into separate vectors
    x = data(:,1);
    y = data(:,2);
    
    % Create a new figure and plot the data
    figure();
    plot(x, y, '-o', 'MarkerSize', 2);
    
    % Label the axes and give the plot a title
    xlabel('Time (s)');
    ylabel('Congestion Window (cwnd)');
    title('TCP Congestion Window Size vs. Time', '( Attack Bots = ' + num_bots + ' )');
    fontsize(gca,40,"pixels")
    set(gcf, 'Position', [100 100 1600 1200]);
    set(gcf, 'Visible', 'off');
    print(folder + '/' + filename + '.png', '-dpng', '-r600');

end