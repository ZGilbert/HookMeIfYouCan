//
//  main.m
//  HookMeIfYouCan
//
//  Created by BlueCocoa on 15/5/3.
//  Copyright (c) 2015年 0xBBC. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "MemoryRegion.h"

@interface  Person : NSObject {
    NSString * _name;
    int _age;
}

@property (strong, nonatomic, readonly) NSString * name;
@property (nonatomic, readonly) int age;

- (instancetype)initWithName:(NSString *)name age:(int)age;

@end

__attribute__((always_inline)) void encryptSuperman(void ** data_ptr, ssize_t length) {
    char * data = (char *) * data_ptr;
    for (ssize_t i = 0; i < length; i++) {
        data[i] ^= 0xBBC - i;
    }
}

__attribute__((always_inline)) void decryptSuperman(void ** data_ptr, ssize_t length) {
    char * data = (char *) * data_ptr;
    for (ssize_t i = 0; i < length; i++) {
        data[i] ^= 0xBBC - i;
    }
}

__attribute__((always_inline)) void setName(void * obj, NSString * newName) {
    void * ptr = (void *)((long)(long *)(obj) + sizeof(Person *));
    memcpy(ptr, (void*) &newName, sizeof(char) * newName.length);
}

__attribute__((always_inline)) void setAge(void * obj, int newAge) {
    void * ptr = (void *)((long)(long *)obj + sizeof(Person *) + sizeof(NSString *));
    memcpy(ptr, &newAge, sizeof(int));
}

@implementation Person

@synthesize name = _name;
@synthesize age = _age;

- (instancetype)initWithName:(NSString *)name age:(int)age{
    self = [self init];
    if (self) {
        _name = name;
        _age = age;
    }
    return self;
}

- (void)setName:(NSString *)name {
    _name = @"Naive";
}

- (void)setAge:(int)age {
    _age = INT32_MAX;
}

- (NSString *)name {
    return @"233";
}

- (int)age {
    return INT32_MIN;
}

- (NSString *)description {
    return [NSString stringWithFormat:@"This person is named %@, aged %d.",self.name,self.age];
}

@end

int main(int argc, const char * argv[]) {
    Person * normal_man = [[Person alloc] initWithName:@"Nobody" age:0];
    
    ssize_t object_size = sizeof(Person *) + sizeof(NSString *) + sizeof(int);
    MemoryRegion mmgr = MemoryRegion(1024);
    void * superman = mmgr.malloc(object_size);
    memcpy(superman, (__bridge void *)normal_man, object_size);
    
    setName(superman, @"Superman");
    setAge (superman, 20);
    
    encryptSuperman(&superman, object_size);

    /**
     *  @brief  为了演示方便加的while
     */
    while (1) {
        NSLog(@"Normal:   %p %@",normal_man,[normal_man name]);
        NSLog(@"Superman: %p",superman);
        
        decryptSuperman(&superman, object_size);
        NSLog(@"This person is named %@, aged %d",*((CFStringRef *)(void*)((long)(long *)(superman) + sizeof(Person *))), *((int *)((long)(long *)superman + sizeof(Person *) + sizeof(NSString *))));
        encryptSuperman(&superman, object_size);
        
        sleep(2);
    }
    return 0;
}

