//
//  MDataSource.m
//  uPINPoint Live
//
//  Created by Nicholas Ver Voort on 3/22/13.
//  Copyright (c) 2013 Engaging Computing Group. All rights reserved.
//

#import "MDataSource.h"
#import "CorePlot.h"

@implementation MDataSource

-(void)setDataArray:(NSArray*)dataArray {
    myDataArray = dataArray;
}

-(NSUInteger)numberOfRecordsForPlot:(CPTPlot *)plot {
    return [myDataArray count];
}

-(NSNumber *)numberForPlot:(CPTPlot *)plot field:(NSUInteger)fieldEnum recordIndex:(NSUInteger)index {
    return [[myDataArray objectAtIndex: index] objectAtIndex: fieldEnum];
}

@end
